#pragma once

#include <stdint.h>

#include "nvs_flash.h"

typedef enum {
    ESPNOW_STATE_NOT_PAIRED = 0,
    ESPNOW_STATE_PAIRING,
    ESPNOW_STATE_PAIRED,
    ESPNOW_STATE_REJOINING
} espnow_state_t;

// setters
void enmod_set_peer_mac(const uint8_t* peer_mac);
void enmod_set_state(espnow_state_t state);

// getters
const uint8_t* enmod_get_peer_mac(void);
bool enmod_has_peer(void);
espnow_state_t enmod_get_state(void);
/**
 * usage:
 * char mac_buf[18];
 * format_mac_str(mac_buf, sizeof(mac_buf), s_peer_mac);
 * ESP_LOGI(TAG, "Peer: %s", mac_buf);
 */ 
void enmod_formatted_peer_addr(char *buf, size_t len);

// basics
const char *enmod_state_to_str(espnow_state_t state);

// callbacks
typedef void (*espnow_on_toggle_cb_t)(void);
typedef void (*espnow_on_state_cb_t)(espnow_state_t new_state);

void enmod_set_state_cb(espnow_on_state_cb_t cb);

// nvs
esp_err_t nvs_save_peer(const uint8_t mac[6]);
bool nvs_peer_exists(void);
bool nvs_load_peer_into_memory(void);