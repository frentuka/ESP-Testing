#include "enmod_basics.h"

#include "esp_err.h"
#include "esp_log.h"

#define NVS_NS "espnow_mod"
#define NVS_KEY_PEER "peer_mac"

#define TAG "ENMOD_BASICS"

static espnow_state_t s_state = ESPNOW_STATE_NOT_PAIRED;
static uint8_t s_peer_mac[6] = {0};

static espnow_on_state_cb_t s_on_state;

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
    s_state = state;
    if (s_on_state != 0) s_on_state(s_state);
}

void enmod_set_state_cb(espnow_on_state_cb_t cb)
{
    s_on_state = cb;
}

/*
    getters
*/
const uint8_t* enmod_get_peer_mac(void)
{
    return s_peer_mac;
}

bool enmod_has_peer(void)
{
    static const uint8_t zero[6] = {0};
    return memcmp(s_peer_mac, zero, 6) != 0;
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

espnow_state_t enmod_get_state(void)
{
    return s_state;
}

/*
    basic utils
*/

const char *enmod_state_to_str(espnow_state_t state)
{
    switch (state) {
        case ESPNOW_STATE_NOT_PAIRED: return "NOT_PAIRED";
        case ESPNOW_STATE_PAIRING:    return "PAIRING";
        case ESPNOW_STATE_PAIRED:     return "PAIRED";
        case ESPNOW_STATE_REJOINING:  return "REJOINING";
        default:                      return "UNKNOWN";
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

    size_t len = sizeof(enmod_get_peer_mac());
    err = nvs_get_blob(h, NVS_KEY_PEER, enmod_get_peer_mac(), &len);
    nvs_close(h);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "peer_mac no encontrado");
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_get_blob fallo=0x%x", err);
        return false;
    }
    if (len != 6) {
        ESP_LOGW(TAG, "tamano inesperado de peer_mac=%u", (unsigned)len);
        return false;
    }


    char mac_buf[18];
    enmod_formatted_peer_addr(mac_buf, sizeof(mac_buf));
    ESP_LOGI(TAG, "peer_mac cargado: %s", mac_buf);
    return true;
}