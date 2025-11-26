#pragma once

#include <stdint.h>
#include "esp_now.h"
#include "nvs_flash.h"

#define ESPNOW_CHANNEL 1
#define ESPNOW_BCAST_MAC_ADDR {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
#define ESPNOW_CONNECTION_TIMEOUT_MS 15000

/*
    states
*/

typedef enum {
    ESPNOW_STATE_IDLE = 1,
    ESPNOW_STATE_DISCOVERING,
    ESPNOW_STATE_SECURING,
    ESPNOW_STATE_RECONNECTING,
    ESPNOW_STATE_CONNECTED,
    ESPNOW_STATE_LINK_LOST,
} espnow_state_t;

/*
    messages
*/

typedef enum {
    MSG_DISCOVERY = 1,
    MSG_DISCOVERY_ACK,

    MSG_SECURE,
    MSG_SECURE_ACK,

    MSG_RECONNECT,
    MSG_RECONNECT_ACK,

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
} secure_pkt_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
} reconnect_pkt_t;

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

// callbacks
typedef void (*espnow_on_toggle_cb_t)(void);
typedef void (*espnow_on_state_cb_t)(espnow_state_t new_state);

// setters
void enmod_set_peer_mac(const uint8_t* peer_mac);
void enmod_set_state(espnow_state_t state);
void enmod_set_state_cb(espnow_on_state_cb_t cb);

esp_err_t enmod_add_peer(uint8_t mac[6], bool encrypt, bool replace_current_peer);
esp_err_t enmod_add_broadcast_peer();
void enmod_del_broadcast_peer();

esp_err_t enmod_delete_all_peers_except(uint8_t *exception);

// getters
const uint8_t* enmod_get_peer_mac(void);
const uint8_t* enmod_get_self_mac(void);
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
const char *enmod_msg_to_str(msg_type_t msg);

void enmod_set_state_cb(espnow_on_state_cb_t cb);

// nvs
esp_err_t nvs_save_peer(const uint8_t mac[6]);
esp_err_t nvs_save_lmk(const uint8_t lmk[ESP_NOW_KEY_LEN]);
bool nvs_peer_exists(void);
bool nvs_load_peer_into_memory(void);
bool nvs_load_lmk(uint8_t *out_lmk);

// secure peers
esp_err_t enmod_add_secure_peer(const uint8_t peer_mac[ESP_NOW_ETH_ALEN], bool replace_current_peer);