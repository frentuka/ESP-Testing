#include <stdio.h>

#include "espnow.h"

#include "esp_mac.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_chip_info.h"

#include "esp_rom_sys.h"
#include "freertos/semphr.h"

#define TAG "ESPNOW_MOD"
#define ESPNOW_PMK "tecleados-pmk-cd"   // 16 caracteres exactos
#define ESPNOW_DISC_MS 200              // intervalo de descubrimiento
#define ESPNOW_CHANNEL 1

#define NVS_NS "espnow_mod"
#define NVS_KEY_PEER "peer_mac"

typedef enum {
    MSG_DISCOVERY = 1,
    MSG_DISCOVERY_ACK,
    MSG_PAIR_REQUEST,
    MSG_PAIR_REQUEST_ACK,
    MSG_TOGGLE,

    MSG_PING,          // nuevo
    MSG_PONG,          // nuevo
    MSG_RATE_START,    // nuevo
    MSG_RATE,          // nuevo
    MSG_RATE_RESULT    // nuevo
} msg_type_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t reserved[7];
} pkt_t;

/*
    STATS PACKETS
*/

typedef struct __attribute__((packed)) {
    uint8_t  type;
    uint32_t seq;
    uint64_t t0_us;   // timestamp origen en microsegundos
} pkt_ping_t;         // usado para PING y PONG

typedef struct __attribute__((packed)) {
    uint8_t  type;
    uint32_t duration_ms; // ventana de conteo en receptor
} pkt_rate_start_t;

typedef struct __attribute__((packed)) {
    uint8_t  type;
    uint32_t seq;
} pkt_rate_t;

typedef struct __attribute__((packed)) {
    uint8_t  type;
    uint32_t count;       // cuántos RATE recibió el peer en su ventana
} pkt_rate_result_t;

//

static espnow_state_t s_state = ESPNOW_STATE_NOT_PAIRED;
static uint8_t s_peer_mac[6] = {0};
static espnow_on_toggle_cb_t s_on_toggle = NULL;
static espnow_on_state_cb_t  s_on_state  = NULL;

// move from here
static bool is_valid_unicast_mac(const uint8_t mac[6]) {
    static const uint8_t ZERO[6]   = {0};
    static const uint8_t BCAST[6]  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    if (!mac) return false;
    if (memcmp(mac, ZERO, 6) == 0) return false;     // todo cero
    if (memcmp(mac, BCAST, 6) == 0) return false;    // broadcast
    if (mac[0] & 0x01)           return false;       // multicast, no unicast
    return true;
}

/*

    > STATISTICS

*/

// espnow_module.h  (añade al final)

typedef struct {
    uint32_t sent;
    uint32_t recv;
    uint32_t lost;        // sent - recv
    uint32_t rtt_min_us;
    uint32_t rtt_max_us;
    uint32_t rtt_avg_us;  // promedio entero
} espnow_pingpong_stats_t;

/**
 * @brief Ping-pong RTT entre peers. Bloqueante.
 * @param count         Número de PINGs a enviar.
 * @param interval_us   Pausa entre PINGs.
 * @param out           Estadísticas resultantes.
 * @return ESP_OK si se recibieron al menos 1 PONG. ESP_FAIL si no hubo respuesta.
 */
esp_err_t espnow_bench_pingpong(uint32_t count, uint32_t interval_us,
                                espnow_pingpong_stats_t *out);

typedef struct {
    uint32_t duration_ms;     // ventana en el receptor
    uint32_t tx_interval_us;  // 0 = lo más rápido posible
    uint32_t rx_count;        // paquetes que el receptor contó
} espnow_rate_result_t;

/**
 * @brief Benchmark de "polling rate" entre peers. Bloqueante.
 * @param duration_ms     Duración de la ventana en el receptor.
 * @param tx_interval_us  Separación entre tramas RATE (0 = saturar).
 * @param out             Resultado con rx_count (paquetes/s = rx_count / (duration_ms/1000)).
 * @return ESP_OK si se recibió el resultado del peer. ESP_FAIL en timeout o sin peer.
 */
esp_err_t espnow_bench_rate(uint32_t duration_ms, uint32_t tx_interval_us,
                            espnow_rate_result_t *out);


static SemaphoreHandle_t s_sem_pong = NULL;     // señal de PONG
static volatile uint32_t s_last_rtt_us = 0;     // RTT del último PONG
static volatile uint32_t s_pong_seq = 0;

static SemaphoreHandle_t s_sem_rate = NULL;     // señal de RATE_RESULT
static volatile uint32_t s_rate_rx_count = 0;   // resultado del peer

// Lado receptor del RATE
static volatile bool     s_rate_window_active = false;
static volatile uint64_t s_rate_window_end_us = 0;
static volatile uint32_t s_rate_window_count  = 0;

static inline uint64_t now_us(void) { return (uint64_t) esp_timer_get_time(); }

esp_err_t espnow_bench_pingpong(uint32_t count, uint32_t interval_us,
                                espnow_pingpong_stats_t *out)
{
    if (s_state != ESPNOW_STATE_PAIRED) return ESP_FAIL;
    if (!out) return ESP_ERR_INVALID_ARG;

    memset(out, 0, sizeof(*out));
    uint32_t rtt_min = UINT32_MAX, rtt_max = 0, rtt_sum = 0, recv = 0;

    for (uint32_t i = 0; i < count; ++i) {
        pkt_ping_t ping = {.type = MSG_PING, .seq = i, .t0_us = now_us()};
        esp_err_t err = esp_now_send(s_peer_mac, (uint8_t*)&ping, sizeof(ping));
        if (err != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
        out->sent++;

        // Espera PONG con timeout corto por iteración
        if (xSemaphoreTake(s_sem_pong, pdMS_TO_TICKS(500)) == pdTRUE && s_pong_seq == i) {
            uint32_t rtt = s_last_rtt_us;
            recv++;
            if (rtt < rtt_min) rtt_min = rtt;
            if (rtt > rtt_max) rtt_max = rtt;
            rtt_sum += rtt;
        }
        if (interval_us) esp_rom_delay_us(interval_us); // evita WDT: usa valores moderados
    }

    out->recv       = recv;
    out->lost       = out->sent - recv;
    out->rtt_min_us = (recv ? rtt_min : 0);
    out->rtt_max_us = (recv ? rtt_max : 0);
    out->rtt_avg_us = (recv ? (rtt_sum / recv) : 0);

    return (recv ? ESP_OK : ESP_FAIL);
}

esp_err_t espnow_bench_rate(uint32_t duration_ms, uint32_t tx_interval_us,
                            espnow_rate_result_t *out)
{
    if (s_state != ESPNOW_STATE_PAIRED) return ESP_FAIL;
    if (!out || duration_ms == 0) return ESP_ERR_INVALID_ARG;

    // 1) Ordena al peer abrir una ventana de conteo
    pkt_rate_start_t start = {.type = MSG_RATE_START, .duration_ms = duration_ms};
    ESP_ERROR_CHECK(esp_now_send(s_peer_mac, (uint8_t*)&start, sizeof(start)));

    // 2) Durante esa ventana, bombardea con RATE
    uint64_t t_end = now_us() + (uint64_t)duration_ms * 1000ULL;
    uint32_t seq = 0;
    while (now_us() < t_end) {
        pkt_rate_t r = {.type = MSG_RATE, .seq = seq++};
        esp_now_send(s_peer_mac, (uint8_t*)&r, sizeof(r));
        if (tx_interval_us) {
            // Ritmo controlado
            esp_rom_delay_us(tx_interval_us);
        } else {
            // “Lo más rápido posible”: aún así añade respiro ocasional
            if ((seq & 0x3FF) == 0) vTaskDelay(0);
        }
    }

    // 3) Espera el RESULT del peer
    if (xSemaphoreTake(s_sem_rate, pdMS_TO_TICKS(1000 + duration_ms)) != pdTRUE) {
        return ESP_FAIL;
    }

    out->duration_ms   = duration_ms;
    out->tx_interval_us= tx_interval_us;
    out->rx_count      = s_rate_rx_count;
    return ESP_OK;
}

/*
    nvs
*/

void ensure_nvs_ready(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
}

static esp_err_t nvs_save_peer(const uint8_t mac[6]) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, NVS_KEY_PEER, mac, 6);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

static bool nvs_load_peer(uint8_t mac_out[6]) {
    nvs_handle_t h;
    size_t len = 6;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;

    esp_err_t err = nvs_get_blob(h, NVS_KEY_PEER, mac_out, &len);
    nvs_close(h);
    if (err != ESP_OK || len != 6) return false;

    // Registrar peer directamente al cargar desde NVS
    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, mac_out, 6);
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;

    esp_now_del_peer(mac_out);  // por si ya existía
    return (esp_now_add_peer(&peer) == ESP_OK);
}

static esp_err_t nvs_erase_peer(void) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_erase_key(h, NVS_KEY_PEER);
    if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

/*
    esp functions
*/

static void set_state(espnow_state_t st, const uint8_t mac[6])
{
    s_state = st;
    if (mac) memcpy(s_peer_mac, mac, 6);
    if (s_on_state) s_on_state(st, mac);
    if (st == ESPNOW_STATE_PAIRED)
        nvs_save_peer(mac);
}

static bool has_peer(void)
{
    static const uint8_t zero[6] = {0};
    return memcmp(s_peer_mac, zero, 6) != 0;
}

static esp_err_t add_peer_if_needed(const uint8_t mac[6])
{
    if (has_peer() && memcmp(s_peer_mac, mac, 6) == 0) return ESP_OK;

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, mac, 6);
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;

    esp_now_del_peer(mac);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    memcpy(s_peer_mac, mac, 6);
    return ESP_OK;
}

/* === CALLBACKS === */

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info,
                           const uint8_t *data, int len)
{
    if (!data || len < 1) return;
    const uint8_t *src = recv_info->src_addr;
    const pkt_t *pkt = (const pkt_t*) data;

    switch (pkt->type) {
    case MSG_DISCOVERY:
        add_peer_if_needed(src);
        set_state(ESPNOW_STATE_PAIRED, src);
        {
            pkt_t ack = {.type = MSG_DISCOVERY_ACK};
            esp_now_send(src, (uint8_t*)&ack, sizeof(ack));
        }
        break;

    case MSG_DISCOVERY_ACK:
        add_peer_if_needed(src);
        set_state(ESPNOW_STATE_PAIRED, src);

        // test ping
        espnow_pingpong_stats_t s;
        if (espnow_bench_pingpong(50, 1000, &s) == ESP_OK) {
            ESP_LOGI("PINGPONG", "sent=%u recv=%u lost=%u rtt[min/avg/max]=%u/%u/%u us",
                    s.sent, s.recv, s.lost, s.rtt_min_us, s.rtt_avg_us, s.rtt_max_us);
        } else {
            ESP_LOGW("PINGPONG", "sin respuesta");
        }

        espnow_rate_result_t r;
        if (espnow_bench_rate(1000, 0, &r) == ESP_OK) {
            float sps = (float)r.rx_count * 1000.0f / (float)r.duration_ms;
            ESP_LOGI("RATE", "rx_count=%u, ~%.1f señales/s", r.rx_count, sps);
        } else {
            ESP_LOGW("RATE", "rate benchmark falló");
        }

        break;

    case MSG_PAIR_REQUEST:
        if (s_state != ESPNOW_STATE_PAIRED && s_peer_mac == src) {
            set_state(ESPNOW_STATE_PAIRED, src);
            pkt_t ack = {.type = MSG_PAIR_REQUEST_ACK};
            esp_now_send(src, (uint8_t*)&ack, sizeof(ack));
        }
        break;

    case MSG_PAIR_REQUEST_ACK:
        if (s_state != ESPNOW_STATE_PAIRED && s_peer_mac == src) {
            // peer should be already added
            set_state(ESPNOW_STATE_PAIRED, src);
        }

    case MSG_TOGGLE:
        if (s_on_toggle) s_on_toggle();
        break;

    /*
        STATISTICS
    */

    case MSG_PING: {
        // eco inmediato con los mismos campos
        if (len >= (int)sizeof(pkt_ping_t)) {
            const pkt_ping_t *p = (const pkt_ping_t*)data;
            pkt_ping_t pong = {.type = MSG_PONG, .seq = p->seq, .t0_us = p->t0_us};
            esp_now_send(recv_info->src_addr, (uint8_t*)&pong, sizeof(pong));
        }
    } break;

    case MSG_PONG: {
        if (len >= (int)sizeof(pkt_ping_t)) {
            const pkt_ping_t *p = (const pkt_ping_t*)data;
            uint32_t rtt = (uint32_t)(now_us() - p->t0_us);
            s_last_rtt_us = rtt;
            s_pong_seq    = p->seq;
            xSemaphoreGive(s_sem_pong);
        }
    } break;

    case MSG_RATE_START: {
        if (len >= (int)sizeof(pkt_rate_start_t)) {
            const pkt_rate_start_t *rs = (const pkt_rate_start_t*)data;
            s_rate_window_active = true;
            s_rate_window_count  = 0;
            s_rate_window_end_us = now_us() + (uint64_t)rs->duration_ms * 1000ULL;
        }
    } break;

    case MSG_RATE: {
        if (s_rate_window_active) {
            s_rate_window_count++;
            if (now_us() >= s_rate_window_end_us) {
                s_rate_window_active = false;
                pkt_rate_result_t out = {.type = MSG_RATE_RESULT, .count = s_rate_window_count};
                esp_now_send(recv_info->src_addr, (uint8_t*)&out, sizeof(out));
            }
        }
    } break;

    case MSG_RATE_RESULT: {
        if (len >= (int)sizeof(pkt_rate_result_t)) {
            const pkt_rate_result_t *rr = (const pkt_rate_result_t*)data;
            s_rate_rx_count = rr->count;
            xSemaphoreGive(s_sem_rate);
        }
    } break;

    default:
        break;
    }
}

static void espnow_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    (void)tx_info->src_addr;
    (void)status;
    ESP_LOGI(TAG, "Sent message: %d", tx_info->data);
}

/*
    init & api
*/

static esp_err_t wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
    return ESP_OK;
}

esp_err_t espnow_module_init(espnow_on_toggle_cb_t on_toggle,
                             espnow_on_state_cb_t on_state)
{
    s_on_toggle = on_toggle;
    s_on_state  = on_state;

    ensure_nvs_ready();

    // init wifi & now
    ESP_ERROR_CHECK(wifi_init_sta());
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_set_pmk((const uint8_t*)ESPNOW_PMK));

    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

    set_state(ESPNOW_STATE_NOT_PAIRED, NULL);

    if (!s_sem_pong) s_sem_pong = xSemaphoreCreateBinary();
    if (!s_sem_rate) s_sem_rate = xSemaphoreCreateBinary();

    // try to connect with existing peer
    // ESP_LOGI(TAG, "Checking for existing peer inside NVS...");
    // uint8_t mac_out[6];
    // if (nvs_load_peer(mac_out)) {
    //     if (!is_valid_unicast_mac(mac_out)) {
    //         if (is_valid_unicast_mac(mac_out)) {

    //             ESP_LOGI(TAG, "Trying to pair with existing peer...");
    //             pkt_t disco = {.type = MSG_PAIR_REQUEST};

    //             while (s_state == ESPNOW_STATE_NOT_PAIRED) {
    //                 ESP_LOGI(TAG, "Pair attempt with existing peer: %s", mac_out);
    //                 esp_now_send(mac_out, (uint8_t*)&disco, sizeof(disco));
    //                 vTaskDelay(pdMS_TO_TICKS(ESPNOW_DISC_MS));
    //             }
    //         }
    //     }
    // }

    return ESP_OK;
}

esp_err_t espnow_module_try_pair(uint32_t ms_timeout)
{
    set_state(ESPNOW_STATE_PAIRING, NULL);

    const uint8_t bcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    
    add_peer_if_needed(bcast);

    pkt_t disco = {.type = MSG_DISCOVERY};
    uint64_t start = esp_timer_get_time() / 1000ULL;

    while ((esp_timer_get_time() / 1000ULL - start) < ms_timeout) {
        if (s_state == ESPNOW_STATE_PAIRED) return ESP_OK;
        esp_now_send(bcast, (uint8_t*)&disco, sizeof(disco));
        vTaskDelay(pdMS_TO_TICKS(ESPNOW_DISC_MS));
        if (s_state == ESPNOW_STATE_PAIRED) {
            esp_now_del_peer(bcast);
            return ESP_OK;
        }
    }

    esp_now_del_peer(bcast);
    set_state(ESPNOW_STATE_NOT_PAIRED, NULL);
    return ESP_ERR_TIMEOUT;
}

esp_err_t espnow_module_send_toggle(void)
{
    if (s_state != ESPNOW_STATE_PAIRED) return ESP_FAIL;
    pkt_t msg = {.type = MSG_TOGGLE};
    return esp_now_send(s_peer_mac, (uint8_t*)&msg, sizeof(msg));
}

espnow_state_t espnow_module_get_state(void)
{
    return s_state;
}

bool espnow_module_get_peer(uint8_t out_mac[6])
{
    if (!has_peer()) return false;
    if (out_mac) memcpy(out_mac, s_peer_mac, 6);
    return true;
}