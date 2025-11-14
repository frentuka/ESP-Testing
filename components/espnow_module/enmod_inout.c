#include "enmod_basics.h"
#include "enmod_inout.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"
#include "esp_log.h"

#include <string.h>

#define TAG "ESPNOW_INOUT"

/*
 *  This  module is meant to control ESPNOW's inputs and outputs
 *  as well as keeping track of it's current state.
 */

#define ESPNOW_PMK "tecleados-pmk420"   // 16 caracteres exactos
#define ESPNOW_DISC_MS 200              // intervalo de descubrimiento
#define ESPNOW_REJOIN_MS 500
#define ESPNOW_CHANNEL 1


static espnow_on_toggle_cb_t s_on_toggle = NULL;

/*
    messages
*/

typedef enum {
    MSG_DISCOVERY = 1,
    MSG_DISCOVERY_ACK,

    MSG_REJOIN,
    MSG_REJOIN_ACK,

    MSG_PING,
    MSG_PONG,

    MSG_TOGGLE,
    MSG_KBSTATE
} msg_type_t;

/*
    packets
*/

typedef struct __attribute__((packed)) {
    uint8_t type;
} discovery_pkt_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
} rejoin_pkt_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint64_t timestamp;
} ping_pkt_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
}  toggle_pkt_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t data[31];
} kbstate_pkt_t;


/*
    setters
*/

// private
static esp_err_t add_peer_if_needed(const uint8_t mac[6])
{
    if (enmod_has_peer()) return ESP_OK;

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, mac, 6);
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;

    esp_now_del_peer(mac);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    
    enmod_set_peer_mac(mac);

    return ESP_OK;
}

void enmod_inout_set_toggle_cb(espnow_on_toggle_cb_t cb)
{
    s_on_toggle = cb;
}

/*
    callbacks
*/

void espnow_module_ping(void)
{
    char mac_buf[18];
    enmod_formatted_peer_addr(mac_buf, sizeof(mac_buf));
    ESP_LOGI(TAG, "Pinging %s", mac_buf);
    
    ping_pkt_t disco = { .type = MSG_PING, .timestamp = esp_timer_get_time() };

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, enmod_get_peer_mac(), 6);
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;

    esp_err_t err_send = esp_now_send(enmod_get_peer_mac(), (uint8_t*)&disco, sizeof(disco));
    if (err_send != ESP_OK) ESP_LOGI(TAG, "Ping failed: %s", esp_err_to_name(err_send));
}

void espnow_module_pong(uint64_t timestamp)
{
    char mac_buf[18];
    enmod_formatted_peer_addr(mac_buf, sizeof(mac_buf));
    ESP_LOGI(TAG, "Ponging %s", mac_buf);
    
    ping_pkt_t disco = { .type = MSG_PONG, .timestamp = timestamp };

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, enmod_get_peer_mac(), 6);
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;

    esp_err_t err_send = esp_now_send(enmod_get_peer_mac(), (uint8_t*)&disco, sizeof(disco));
    if (err_send != ESP_OK) ESP_LOGI(TAG, "Pong failed: %s", esp_err_to_name(err_send));
}

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info,
                           const uint8_t *data, int len)
{
    if (!data || len < 1) return;
    const uint8_t *src = recv_info->src_addr;
    msg_type_t msg_type = data[0];

    ESP_LOGI(TAG, "Received message from %02X:%02X:%02X:%02X:%02X:%02X",
             src[0], src[1], src[2],
             src[3], src[4], src[5]);

    switch (msg_type) {
    case MSG_DISCOVERY:
        // do not pair if not pairing
        if (enmod_get_state() != ESPNOW_STATE_PAIRING) return;

        add_peer_if_needed(src);
        enmod_set_state(ESPNOW_STATE_PAIRED);
        enmod_set_peer_mac(src);

        {
            // send ack
            discovery_pkt_t ack = {.type = MSG_DISCOVERY_ACK};
            esp_now_send(src, (uint8_t*)&ack, sizeof(ack));

            // store peer
            nvs_save_peer(src);
        }

        break;

    case MSG_DISCOVERY_ACK:
        // do not pair if not pairing
        if (enmod_get_state() != ESPNOW_STATE_PAIRING) return;

        // store peer
        nvs_save_peer(src);

        // add as peer
        add_peer_if_needed(src);
        enmod_set_state(ESPNOW_STATE_PAIRED);
        enmod_set_peer_mac(src);
        break;

    case MSG_REJOIN:
        if (memcmp(enmod_get_peer_mac(), src, 6) == 0) {
            ESP_LOGI(TAG, "Received valid rejoin request");
            enmod_set_state(ESPNOW_STATE_PAIRED);
            enmod_set_peer_mac(src);
            {
                rejoin_pkt_t ack = {.type = MSG_REJOIN_ACK};
                esp_now_send(src, (uint8_t*)&ack, sizeof(ack));
            }
        } else {
            ESP_LOGI(TAG, "Received INVALID rejoin request");
        }
        break;

    case MSG_REJOIN_ACK:
        if (!memcmp(enmod_get_peer_mac(), src, 6) == 0 || enmod_get_state() != ESPNOW_STATE_REJOINING) return;
        enmod_set_state(ESPNOW_STATE_PAIRED);
        break;
    
    case MSG_PING:
    {
        if (!memcmp(enmod_get_peer_mac(), src, 6) == 0 || enmod_get_state() != ESPNOW_STATE_PAIRED) return;
        ping_pkt_t *pkt = (ping_pkt_t*) data;
        const uint64_t timestamp = pkt->timestamp;
        espnow_module_pong((unsigned long long)timestamp);
        break;
    }
    
    case MSG_PONG:
    {
        ESP_LOGI(TAG, "Pong-received conditions: %d, %d", memcmp(enmod_get_peer_mac(), src, 6), enmod_state_to_str(enmod_get_state()));
        if (!memcmp(enmod_get_peer_mac(), src, 6) == 0 || enmod_get_state() != ESPNOW_STATE_PAIRED) return;
        ping_pkt_t *pkt = (ping_pkt_t*) data;
        uint64_t ping_timestamp = pkt->timestamp / 1000; // ms
        uint64_t now = esp_timer_get_time() / 1000; // ms
        uint64_t rtt = now - ping_timestamp;
        ESP_LOGI(TAG, "Pong received! Now: %llu, Then: %llu, Travel time: %llu",
            (unsigned long long) now,
            (unsigned long long) ping_timestamp,
            (unsigned long long) rtt);
        break;
    }

    case MSG_TOGGLE:
        if (s_on_toggle) s_on_toggle();
        break;

    default:
        break;
    }
}

static void espnow_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    (void)tx_info->src_addr;
    (void)status;
    ESP_LOGI(TAG, "Sent message to %s", tx_info->des_addr);
}

/*
    esp stuff
*/

esp_err_t enmod_inout_send_toggle(void)
{
    ESP_LOGI(TAG, "Toggle signal sent");
    if (enmod_get_state() != ESPNOW_STATE_PAIRED) return ESP_FAIL;
    toggle_pkt_t msg = {.type = MSG_TOGGLE};
    return esp_now_send(enmod_get_peer_mac(), (uint8_t*)&msg, sizeof(msg));
}

/*
    pairing
*/

void enmod_inout_try_rejoin(uint32_t timeout) {
    enmod_set_state(ESPNOW_STATE_REJOINING);
    vTaskDelay(pdMS_TO_TICKS(1000)); // 1 segundo de drama. quitar

    char mac_buf[18];
    enmod_formatted_peer_addr(mac_buf, sizeof(mac_buf));
    ESP_LOGI(TAG, "Trying to rejoin existing peer: %s", mac_buf);

    // packet
    rejoin_pkt_t disco = {.type = MSG_REJOIN};

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, enmod_get_peer_mac(), 6);
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;

    if (!esp_now_is_peer_exist(peer.peer_addr)) {
        esp_err_t err_add = esp_now_add_peer(&peer); // ESP_ERR_ESPNOW_EXIST -> peer has been already added
        if (err_add != ESP_OK && err_add != ESP_ERR_ESPNOW_EXIST) ESP_LOGI(TAG, "Peer add error: %s", esp_err_to_name(err_add));
    }

    uint64_t end_timestamp = esp_timer_get_time() + (timeout * 1000);

    while (enmod_get_state() == ESPNOW_STATE_REJOINING && end_timestamp > esp_timer_get_time()) {
        esp_err_t err_send = esp_now_send(enmod_get_peer_mac(), (uint8_t*)&disco, sizeof(disco));
        if (err_send != ESP_OK) ESP_LOGI(TAG, "Peer send error: %s", esp_err_to_name(err_send));

        vTaskDelay(pdMS_TO_TICKS(ESPNOW_REJOIN_MS));
    }

    if (enmod_get_state() != ESPNOW_STATE_PAIRED)
        enmod_set_state(ESPNOW_STATE_NOT_PAIRED);
}

esp_err_t enmod_inout_discovery_pair(uint32_t ms_timeout)
{
    enmod_set_state(ESPNOW_STATE_PAIRING);

    const uint8_t bcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    
    add_peer_if_needed(bcast);

    discovery_pkt_t disco = {.type = MSG_DISCOVERY};
    uint64_t start = esp_timer_get_time() / 1000ULL;

    uint64_t end_timestamp = esp_timer_get_time() + ms_timeout * 1000;
    while (end_timestamp > esp_timer_get_time()) {
        if (enmod_get_state() == ESPNOW_STATE_PAIRED) return ESP_OK;
        esp_now_send(bcast, (uint8_t*)&disco, sizeof(disco));
        vTaskDelay(pdMS_TO_TICKS(ESPNOW_DISC_MS));
        if (enmod_get_state() == ESPNOW_STATE_PAIRED) {
            esp_now_del_peer(bcast);
            return ESP_OK;
        }
    }

    esp_now_del_peer(bcast);
    enmod_set_state(ESPNOW_STATE_NOT_PAIRED);
    return ESP_ERR_TIMEOUT;
}

void enmod_inout_init(void)
{
    esp_now_set_pmk((const uint8_t*)ESPNOW_PMK);
    esp_now_register_recv_cb(espnow_recv_cb);
    esp_now_register_send_cb(espnow_send_cb);
}