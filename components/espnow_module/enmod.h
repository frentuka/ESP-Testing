#pragma once

#include "enmod_inout.h"

#include <stdbool.h>
#include "esp_err.h"
#include "esp_mac.h"

const char *enmod_state_to_str(espnow_state_t state);

// get
espnow_state_t enmod_get_state(void);

// Sets
void enmod_set_status_cb(espnow_on_state_cb_t cb);
void enmod_set_toggle_cb(espnow_on_toggle_cb_t cb);

// nvs
esp_err_t nvs_save_peer(const uint8_t mac[6]);
bool nvs_peer_exists(void);
bool nvs_load_peer_into_memory(void);

// Inicializa Wi-Fi STA y ESP-NOW. Registra callbacks.
esp_err_t enmod_init(void);

// Intenta emparejar “con quien pueda” durante `ms_timeout`. Devuelve en cuanto empareja o al vencer el tiempo.
esp_err_t enmod_try_pair(uint32_t ms_timeout);

// Envía al par el comando “toggle”.
esp_err_t enmod_send_toggle(void);


// Consulta de estado y MAC del peer.
espnow_state_t enmod_get_state(void);
bool enmod_get_peer(uint8_t out_mac[6]);


void enmod_discovery_pair(uint32_t ms_timeout);
void enmod_ping(void);