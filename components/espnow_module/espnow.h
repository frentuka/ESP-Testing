#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_mac.h"

typedef enum {
    ESPNOW_STATE_NOT_PAIRED = 0,
    ESPNOW_STATE_PAIRING = 1,
    ESPNOW_STATE_PAIRED = 2
} espnow_state_t;

// Callbacks opcionales
typedef void (*espnow_on_toggle_cb_t)(void);
typedef void (*espnow_on_state_cb_t)(espnow_state_t new_state, const uint8_t peer_mac[6]);

// Inicializa Wi-Fi STA y ESP-NOW. Registra callbacks.
esp_err_t espnow_module_init(espnow_on_toggle_cb_t on_toggle,
                             espnow_on_state_cb_t on_state);

// Intenta emparejar “con quien pueda” durante `ms_timeout`. Devuelve en cuanto empareja o al vencer el tiempo.
esp_err_t espnow_module_try_pair(uint32_t ms_timeout);

// Envía al par el comando “toggle”.
esp_err_t espnow_module_send_toggle(void);

// Consulta de estado y MAC del peer.
espnow_state_t espnow_module_get_state(void);
bool espnow_module_get_peer(uint8_t out_mac[6]);

// ping
void ping();