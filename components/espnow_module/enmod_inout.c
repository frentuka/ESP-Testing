#include "enmod_inout.h"
#include "enmod_basics.h"

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

#define ESPNOW_DISC_MS 200              // intervalo de descubrimiento
#define ESPNOW_REJOIN_MS 500

/*
    send messages
    you shall not modify state here
*/

esp_err_t enmod_inout_send_unencrypted_unicast_to_non_paired_peer(const uint8_t *peer_mac, const uint8_t *data)
{
    // add peer as unencrypted peer
    esp_err_t err = enmod_add_peer(peer_mac, false, false);
    if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) return err;

    // send message
    err = esp_now_send(peer_mac, data, sizeof(data));
    if (err != ESP_OK) {
        esp_now_del_peer(peer_mac);
        return err;
    }

    // graceful ending
    esp_now_del_peer(peer_mac);
    return ESP_OK;
}

esp_err_t enmod_inout_send_discovery(void)
{
    // include broadcast peer (0xff)
    const uint8_t bcast[6] = ESPNOW_BCAST_MAC_ADDR;
    enmod_add_broadcast_peer();

    // create packet
    discovery_pkt_t pkt = { .type = MSG_DISCOVERY };

    // send packet
    esp_err_t err = esp_now_send(bcast, (uint8_t*) &pkt, sizeof(pkt));
    ESP_LOGI(TAG, "Sent %s (%s)", enmod_msg_to_str(pkt.type), esp_err_to_name(err));

    enmod_del_broadcast_peer();
    return err;
}

esp_err_t enmod_inout_send_discovery_ack(void)
{
    // include broadcast peer (0xff)
    const uint8_t bcast[6] = ESPNOW_BCAST_MAC_ADDR;
    enmod_add_broadcast_peer();

    // create packet
    discovery_pkt_t pkt = { .type = MSG_DISCOVERY_ACK };

    // send packet
    esp_err_t err = esp_now_send(bcast, (uint8_t*) &pkt, sizeof(pkt));
    ESP_LOGI(TAG, "Sent %s (%s)", enmod_msg_to_str(pkt.type), esp_err_to_name(err));

    enmod_del_broadcast_peer();
    return err;
}

esp_err_t enmod_inout_send_secure(const uint8_t *peer_mac)
{
    // create packet
    secure_pkt_t pkt = { .type = MSG_SECURE };

    // send packet
    esp_err_t err = esp_now_send(peer_mac, (uint8_t*) &pkt, sizeof(pkt));
    ESP_LOGI(TAG, "Sent %s (%s)", enmod_msg_to_str(pkt.type), esp_err_to_name(err));
    return err;
}

esp_err_t enmod_inout_send_secure_ack(void)
{
    // create packet
    secure_pkt_t pkt = { .type = MSG_SECURE_ACK };

    // send packet
    esp_err_t err = esp_now_send(enmod_get_peer_mac(), (uint8_t*) &pkt, sizeof(pkt));
    ESP_LOGI(TAG, "Sent %s (%s)", enmod_msg_to_str(pkt.type), esp_err_to_name(err));
    return err;
}

esp_err_t enmod_inout_send_reconnect(void)
{
    // create packet
    reconnect_pkt_t pkt = { .type = MSG_RECONNECT };

    // send packet
    esp_err_t err = esp_now_send(enmod_get_peer_mac(), (uint8_t*) &pkt, sizeof(pkt));
    ESP_LOGI(TAG, "Sent %s (%s)", enmod_msg_to_str(pkt.type), esp_err_to_name(err));
    return err;
}

esp_err_t enmod_inout_send_reconnect_ack(void)
{
    // create packet
    reconnect_pkt_t pkt = { .type = MSG_RECONNECT_ACK };

    // send packet
    esp_err_t err = esp_now_send(enmod_get_peer_mac(), (uint8_t*) &pkt, sizeof(pkt));
    ESP_LOGI(TAG, "Sent %s (%s)", enmod_msg_to_str(pkt.type), esp_err_to_name(err));
    return err;
}

void enmod_inout_send_ping(void)
{
    // create packet
    ping_pkt_t pkt = { .type = MSG_PING, .timestamp = esp_timer_get_time() };

    // send packet
    esp_err_t err = esp_now_send(enmod_get_peer_mac(), (uint8_t*)&pkt, sizeof(pkt));
    if (err != ESP_OK) ESP_LOGI(TAG, "Ping failed: %s", esp_err_to_name(err));
}

void enmod_inout_send_pong(uint64_t timestamp)
{
    // create packet
    ping_pkt_t pkt = { .type = MSG_PONG, .timestamp = timestamp };

    // send packet
    esp_err_t err = esp_now_send(enmod_get_peer_mac(), (uint8_t*)&pkt, sizeof(pkt));
    if (err != ESP_OK) ESP_LOGI(TAG, "Pong failed: %s", esp_err_to_name(err));
}