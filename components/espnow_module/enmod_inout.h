#pragma once

#include <stdint.h>

#include "enmod_basics.h"

#include "esp_now.h"

/*
    messages
*/

esp_err_t enmod_inout_send_unencrypted_unicast_to_non_paired_peer(const uint8_t *peer_mac, const uint8_t *data);

esp_err_t enmod_inout_send_discovery(void);
esp_err_t enmod_inout_send_discovery_ack(void);

esp_err_t enmod_inout_send_secure(const uint8_t *peer_mac);
esp_err_t enmod_inout_send_secure_ack(void);

esp_err_t enmod_inout_send_reconnect(void);
esp_err_t enmod_inout_send_reconnect_ack(void);

void enmod_inout_send_ping(void);
void enmod_inout_send_pong(uint64_t timestamp);

void enmod_inout_init(void);