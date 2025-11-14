#pragma once

#include <stdint.h>

#include "enmod_basics.h"

#include "esp_now.h"

/*
    getters
*/

const uint8_t* enmod_inout_get_peer_addr(void);
const espnow_state_t enmod_inout_get_state(void);
bool enmod_inout_has_peer(void); 

/*
    setters
*/

void enmod_inout_set_state_cb(espnow_on_state_cb_t cb);
void enmod_inout_set_toggle_cb(espnow_on_toggle_cb_t cb);

/*
    callbacks
*/

void enmod_inout_ping();
//void espnow_module_pong(uint64_t timestamp); <- private

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
static void espnow_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status);

/*
    esp stuff
*/

esp_err_t enmod_inout_send_toggle(void);

/*
    pairing
*/

void enmod_inout_try_rejoin(uint32_t timeout);
esp_err_t enmod_inout_discovery_pair(uint32_t ms_timeout);
void enmod_inout_init(void);