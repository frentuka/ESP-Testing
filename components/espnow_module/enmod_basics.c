#include "enmod_basics.h"

#include "esp_now.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_mac.h"

#include "mbedtls/md.h"
#include "mbedtls/sha512.h"

#define NVS_NS "espnow_mod"
#define NVS_KEY_PEER "peer_mac"
#define NVS_KEY_LMK "lmk"

#define TAG "ENMOD_BASICS"

const unsigned char ESPNOW_PMK[ESP_NOW_KEY_LEN] = "tecleados-pmk420";

static espnow_state_t s_state = ESPNOW_STATE_IDLE;
static uint8_t s_peer_mac[6] = {0};

static espnow_on_state_cb_t s_on_state;
static uint8_t last_state_change_timestamp_ms = 0;

/*
    setters
*/
void enmod_set_peer_mac(const uint8_t* peer_mac)
{
    if (!peer_mac) {
        return;
    }

    memcpy(s_peer_mac, peer_mac, sizeof(s_peer_mac));
}

void enmod_set_state(espnow_state_t state)
{
    bool state_changed = s_state != state;
    
    s_state = state;
    if (s_on_state != 0 && state_changed) {
        s_on_state(s_state);
        last_state_change_timestamp_ms = esp_timer_get_time() / 1000; // us -> ms
    }
}

void enmod_set_state_cb(espnow_on_state_cb_t cb)
{
    s_on_state = cb;
}

esp_err_t enmod_add_peer(uint8_t mac[6], bool encrypt, bool replace_current_peer)
{
    if (encrypt) {
        return enmod_add_secure_peer(mac, replace_current_peer);
    }

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, mac, 6);
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;

    esp_now_del_peer(mac);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    
    if (replace_current_peer) enmod_set_peer_mac(mac);

    return ESP_OK;
}

esp_err_t enmod_add_broadcast_peer()
{
    const uint8_t mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    
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

void enmod_del_broadcast_peer()
{
    const uint8_t mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    esp_now_del_peer(mac);
}

/**
 *  will delete all peers excepting 'exception'
 */
esp_err_t enmod_delete_all_peers_except(uint8_t *exception)
{
    esp_now_peer_info_t peer;
    uint8_t error_count = 0;

    // will forst span bottom-up. if exception is found, will span top-down
    bool from_head = false;

    while (true) {
        // get peer
        esp_err_t err = esp_now_fetch_peer(from_head, &peer);
        if (err != ESP_OK) return err;

        if (memcmp(peer.peer_addr, exception, ESP_NOW_ETH_ALEN) != 0) {
            err = esp_now_del_peer(peer.peer_addr);
        } else {
            if (!from_head) {
                from_head = true;
            } else {
                // end
                return ESP_OK;
            }
        }

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete peer %02X:%02X:%02X:%02X:%02X:%02X (%s)",
                peer.peer_addr[0], peer.peer_addr[1], peer.peer_addr[2],
                peer.peer_addr[3], peer.peer_addr[4], peer.peer_addr[5],
                esp_err_to_name(err));

            // prevent infinite error loops
            error_count++;
            if (error_count > 5) return ESP_ERR_ESPNOW_INTERNAL;
        }
    }

    return ESP_OK;
}

/*
    getters
*/
const uint8_t* enmod_get_peer_mac(void)
{
    return s_peer_mac;
}

static uint8_t s_self_mac[6] = {0};
static bool initialized = false;
static void init_self_mac(void)
{
    // Intentar obtener la MAC desde WiFi primero (IEEE802.11 interface)
    esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, s_self_mac);
    if (err != ESP_OK) {
        // Si falla, usar esp_base_mac_addr_get()
        err = esp_base_mac_addr_get(s_self_mac);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get base MAC (0x%x)", err);
        }
    }
}
const uint8_t* enmod_get_self_mac(void)
{
    if (!initialized) {
        init_self_mac();
        initialized = true;
    }

    return s_self_mac;  // Devuelve puntero constante a la MAC
}

bool enmod_has_peer(void)
{
    static const uint8_t zero[6] = {0};
    return memcmp(s_peer_mac, zero, 6) != 0;
}

espnow_state_t enmod_get_state(void)
{
    return s_state;
}

uint8_t enmod_get_last_state_change_timestamp()
{
    return last_state_change_timestamp_ms;
}

/**
 * usage:
 * char mac_buf[18];
 * enmod_formatted_peer_addr(mac_buf, sizeof(mac_buf));
 * ESP_LOGI(TAG, "Peer: %s", mac_buf);
 */ 
void enmod_formatted_peer_addr(char *buf, size_t len) {
    snprintf(buf, len, "%02X:%02X:%02X:%02X:%02X:%02X",
             s_peer_mac[0], s_peer_mac[1], s_peer_mac[2], s_peer_mac[3], s_peer_mac[4], s_peer_mac[5]);
}

/*
    basic utils
*/

const char *enmod_state_to_str(espnow_state_t state)
{
    switch (state) {
        case ESPNOW_STATE_IDLE:         return "IDLE";
        case ESPNOW_STATE_DISCOVERING:  return "DISCOVERING";
        case ESPNOW_STATE_SECURING:     return "SECURING";
        case ESPNOW_STATE_RECONNECTING: return "RECONNECTING";
        case ESPNOW_STATE_CONNECTED:    return "CONNECTED";
        case ESPNOW_STATE_LINK_LOST:    return "LINK_LOST";
        default:                        return "UNKNOWN";
    }
}

const char *enmod_msg_to_str(msg_type_t msg)
{
    switch (msg) {
        case MSG_DISCOVERY:     return "DISCOVERY";
        case MSG_DISCOVERY_ACK: return "DISCOVERY_ACK";

        case MSG_SECURE:        return "SECURE";
        case MSG_SECURE_ACK:    return "SECURE_ACK";

        case MSG_RECONNECT:     return "RECONNECT";
        case MSG_RECONNECT_ACK: return "RECONNECT_ACK";

        case MSG_PING:          return "PING";
        case MSG_PONG:          return "PONG";

        default: return "UNKNOWN";
    }
}

/*
    nvs
*/

esp_err_t nvs_save_peer(const uint8_t mac[6]) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, NVS_KEY_PEER, mac, 6);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_save_lmk(const uint8_t lmk[ESP_NOW_KEY_LEN]) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, NVS_KEY_LMK, lmk, 6);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

bool nvs_peer_exists(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "nvs_open RO fallo=0x%x", err);
        }
        return false;
    }

    size_t len = 0;
    err = nvs_get_blob(h, NVS_KEY_PEER, NULL, &len);
    nvs_close(h);

    if (err == ESP_ERR_NVS_NOT_FOUND) return false;
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_get_blob(size) fallo=0x%x", err);
        return false;
    }
    return len == 6;
}

bool nvs_load_peer_into_memory(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open RO fallo=0x%x", err);
        return false;
    }

    size_t len = ESP_NOW_ETH_ALEN;
    err = nvs_get_blob(h, NVS_KEY_PEER, s_peer_mac, &len);
    nvs_close(h);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "peer_mac not found inside NVS");
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_get_blob failed (0x%x)", err);
        return false;
    }
    if (len != 6) {
        ESP_LOGW(TAG, "unexpected size for peer_mac (%u)", (unsigned)len);
        return false;
    }

    return true;
}

bool nvs_load_lmk(uint8_t *out_lmk)
{
    if (!out_lmk) {
        ESP_LOGW(TAG, "nvs_load_lmk: out_lmk is NULL");
        return false;
    }

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open RO fallo=0x%x", err);
        return false;
    }

    size_t len = ESP_NOW_KEY_LEN;
    err = nvs_get_blob(h, NVS_KEY_LMK, out_lmk, &len);
    nvs_close(h);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "lmk not found inside NVS");
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_get_blob failed (0x%x)", err);
        return false;
    }
    if (len != ESP_NOW_KEY_LEN) {
        ESP_LOGW(TAG, "unexpected size for lmk (%u)", (unsigned)len);
        return false;
    }

    return true;
}

/*
    secure peer pairing
*/

// private
static esp_err_t espnow_derive_lmk_from_macs(const uint8_t mac1[6], const uint8_t mac2[6], uint8_t out_lmk[ESP_NOW_KEY_LEN])
{
    if (!mac1 || !mac2 || !out_lmk) {
        return ESP_ERR_INVALID_ARG;
    }

    // order is deterministic and independent of who is "mac1" and who is "mac2"
    const uint8_t *mac_low  = mac1;
    const uint8_t *mac_high = mac2;

    if (memcmp(mac1, mac2, 6) > 0) {
        mac_low  = mac2;
        mac_high = mac1;
    }

    //    [ "ESPNOW_LMK" || mac_low || mac_high ]
    uint8_t input[9 + 6 + 6];
    size_t  offset = 0;

    const char *label = "ESPNOW_LMK";
    memcpy(input + offset, label, 9);
    offset += 9;

    memcpy(input + offset, mac_low, 6);
    offset += 6;

    memcpy(input + offset, mac_high, 6);
    offset += 6;

    // Por seguridad
    if (offset != sizeof(input)) {
        return ESP_FAIL; // shouldn't happen
    }
    //    out_full will have 32 bytes, only first 16 will be used
    uint8_t out_full[32];

    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *info;

    mbedtls_md_init(&ctx);

    info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (info == NULL) {
        mbedtls_md_free(&ctx);
        return ESP_FAIL;
    }

    if (mbedtls_md_setup(&ctx, info, 1) != 0) { // 1 = HMAC
        mbedtls_md_free(&ctx);
        return ESP_FAIL;
    }

    if (mbedtls_md_hmac_starts(&ctx, ESPNOW_PMK, ESP_NOW_KEY_LEN) != 0 ||
        mbedtls_md_hmac_update(&ctx, input, sizeof(input)) != 0 ||
        mbedtls_md_hmac_finish(&ctx, out_full) != 0) {

        mbedtls_md_free(&ctx);
        return ESP_FAIL;
    }

    mbedtls_md_free(&ctx);

    // truncate first 16 bytes as LMK
    memcpy(out_lmk, out_full, ESP_NOW_KEY_LEN);

    // clean sensible data from memory
    memset(out_full, 0, sizeof(out_full));
    memset(input, 0, sizeof(input));

    return ESP_OK;
}

esp_err_t enmod_add_secure_peer(const uint8_t peer_mac[ESP_NOW_ETH_ALEN], bool replace_current_peer)
{
    if (!peer_mac) {
        return ESP_ERR_INVALID_ARG;
    }

    // get lmk
    uint8_t lmk[ESP_NOW_KEY_LEN] = {0};

    esp_err_t err = espnow_derive_lmk_from_macs(enmod_get_self_mac(), peer_mac, lmk);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "derive_lmk failed: %s", esp_err_to_name(err));
        return err;
    }

    // create peer struct
    esp_now_peer_info_t peer_info = {0};

    memcpy(peer_info.peer_addr, peer_mac, ESP_NOW_ETH_ALEN);
    peer_info.channel = ESPNOW_CHANNEL;
    peer_info.ifidx   = WIFI_IF_STA;
    peer_info.encrypt = true;

    memcpy(peer_info.lmk, lmk, ESP_NOW_KEY_LEN);

    // register peer
    err = esp_now_add_peer(&peer_info);

    // solve "list full" error
    if (err == ESP_ERR_ESPNOW_FULL) {
        enmod_delete_all_peers_except(enmod_get_peer_mac());
        err = esp_now_add_peer(&peer_info);
    }

    if (err == ESP_ERR_ESPNOW_EXIST) {
        ESP_LOGW(TAG, "Peer already exists, updating LMK");
        esp_now_mod_peer(&peer_info);
        err = ESP_OK;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_add_peer failed (%s)", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG,
             "Secure peer added (%02X:%02X:%02X:%02X:%02X:%02X)",
             peer_mac[0], peer_mac[1], peer_mac[2],
             peer_mac[3], peer_mac[4], peer_mac[5]);

    return ESP_OK;
}