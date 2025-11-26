#include "enmod_machine.h"
#include "enmod_basics.h"
#include "enmod_inout.h"

#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "FreeRTOS/semphr.h"

#include "esp_log.h"
#include "esp_timer.h"

#define TAG "ESPNOW_MAIN"

#define IDLE_INTERVAL_MS         500
#define DISCOVERING_INTERVAL_MS  200
#define CONNECTING_INTERVAL_MS   200
#define RECONNECTING_INTERVAL_MS 100
#define CONNECTED_INTERVAL_MS    500
#define LINK_LOST_INTERVAL_MS    100

static uint8_t securing_peer_mac[6] = {0};
static uint64_t last_pingpong_timestamp_ms = 0;

/*
    states
*/

// idle
static void enmod_machine_frame_IDLE(void)
{
    // idling af
    
    // (do not) do stuff
}

// discovering
static void enmod_machine_frame_DISCOVERING(void)
{
    enmod_inout_send_discovery();
}

// securing
static void enmod_machine_frame_SECURING(void)
{
    static const uint8_t zero_mac[6] = {0};
    if (memcmp(securing_peer_mac, zero_mac, 6) == 0)
        enmod_inout_send_secure(securing_peer_mac);
}

// reconnecting
static void enmod_machine_frame_RECONNECTING(void)
{
    enmod_inout_send_reconnect();
}

// connected
static void enmod_machine_frame_CONNECTED(void)
{
    enmod_inout_send_ping();

    if (last_pingpong_timestamp_ms != 0) {
        uint8_t elapsed_time = esp_timer_get_time()/1000 - last_pingpong_timestamp_ms;
        if (elapsed_time > ESPNOW_CONNECTION_TIMEOUT_MS) {
            enmod_set_state(ESPNOW_STATE_LINK_LOST);
        }
    }
}

// link_lost
static void enmod_machine_frame_LINK_LOST(void)
{
    enmod_inout_send_reconnect();
}

/*
    main task
*/

void enmod_machine_main_task(void *arg)
{
    while (true) {
        switch (enmod_get_state()) {
            case ESPNOW_STATE_IDLE:
                enmod_machine_frame_IDLE();
                vTaskDelay(pdMS_TO_TICKS(IDLE_INTERVAL_MS));
                break;
            case ESPNOW_STATE_DISCOVERING:
                enmod_machine_frame_DISCOVERING();
                vTaskDelay(pdMS_TO_TICKS(DISCOVERING_INTERVAL_MS));
                break;
            case ESPNOW_STATE_SECURING:
                enmod_machine_frame_SECURING();
                vTaskDelay(pdMS_TO_TICKS(CONNECTING_INTERVAL_MS));
                break;
            case ESPNOW_STATE_RECONNECTING:
                enmod_machine_frame_RECONNECTING();
                vTaskDelay(pdMS_TO_TICKS(RECONNECTING_INTERVAL_MS));
                break;
            case ESPNOW_STATE_CONNECTED:
                enmod_machine_frame_CONNECTED();
                vTaskDelay(pdMS_TO_TICKS(CONNECTED_INTERVAL_MS));
                break;
            case ESPNOW_STATE_LINK_LOST:
                enmod_machine_frame_LINK_LOST();
                vTaskDelay(pdMS_TO_TICKS(LINK_LOST_INTERVAL_MS));
                break;
        }
    }
}

/*
    DISCOVERY
*/

void enmod_machine_recv_DISCOVERY(const uint8_t *src, const uint8_t *data)
{
    // ignore if not discovering
    if (enmod_get_state() != ESPNOW_STATE_DISCOVERING) return;
    // state is DISCOVERING

    // step 1: add source as encrypted peer
    enmod_add_secure_peer(src, false);

    // step 2: set state to SECURING
    enmod_set_state(ESPNOW_STATE_SECURING);

    // step 3: answer to BROADCAST with DISCOVERY_ACK
    enmod_inout_send_discovery_ack();
}

void enmod_machine_recv_DISCOVERY_ACK(const uint8_t *src, const uint8_t *data)
{
    // ignore if not discovering
    if (enmod_get_state() != ESPNOW_STATE_DISCOVERING) return;
    // state is DISCOVERING

    // step 1: add source as encrypted peer
    esp_err_t err = enmod_add_secure_peer(src, false);
    ESP_LOGI(TAG, "Securing: added encrypted peer (%s)", esp_err_to_name(err));

    // step 2: set state to SECURING
    enmod_set_state(ESPNOW_STATE_SECURING);
    memcpy(securing_peer_mac, src, 6);

    // step 3: answer to src with SECURE
    enmod_inout_send_secure(src);
}

/*
    SECURE
*/

void enmod_machine_recv_SECURE(const uint8_t *src, const uint8_t *data)
{
    // ignore if not securing
    if (enmod_get_state() != ESPNOW_STATE_SECURING) return;
    // state is SECURING

    // step 1: replace main peer with newly paired peer
    enmod_set_peer_mac(src);

    // step 2: set state to CONNECTED
    enmod_set_state(ESPNOW_STATE_CONNECTED);

    // step 3: answer to src with SECURE_ACK
    esp_err_t err = enmod_inout_send_secure_ack();

    // step 4: clear securing mac address
    memset(securing_peer_mac, 0, sizeof(securing_peer_mac));
}

void enmod_machine_recv_SECURE_ACK(const uint8_t *src, const uint8_t *data)
{
    // ignore if not securing
    if (enmod_get_state() != ESPNOW_STATE_SECURING) return;
    // state is SECURING

    // step 1: replace main peer with newly paired peer
    enmod_set_peer_mac(src);

    // step 2: set state to CONNECTED
    enmod_set_state(ESPNOW_STATE_CONNECTED);

    // step 3: clear securing mac address
    memset(securing_peer_mac, 0, sizeof(securing_peer_mac));

    // no answer needed. connection is established
}

/*
    RECONNECT
*/

void enmod_machine_recv_RECONNECT(const uint8_t *src, const uint8_t *data)
{
    // no state check is needed
    // check src == current peer
    if (memcmp(src, enmod_get_peer_mac(), ESP_NOW_ETH_ALEN) != 0) return;

    // step 1: set state to CONNECTED (probably it already was)
    enmod_set_state(ESPNOW_STATE_CONNECTED);

    // step 2: answer with RECONNECT_ACK
    enmod_inout_send_reconnect_ack();
}

void enmod_machine_recv_RECONNECT_ACK(const uint8_t *src, const uint8_t *data)
{
    // check src == current peer
    if (memcmp(src, enmod_get_peer_mac(), ESP_NOW_ETH_ALEN) != 0) return;

    // no reconnect needed if already connected
    if (enmod_get_state() == ESPNOW_STATE_CONNECTED) return;

    // step 1: set state to CONNECTED
    enmod_set_state(ESPNOW_STATE_CONNECTED);
}

/*
    PING-PONG
*/

void enmod_machine_recv_PING(const uint8_t *src, const uint64_t timestamp)
{
    // check src == current peer
    if (memcmp(src, enmod_get_peer_mac(), ESP_NOW_ETH_ALEN) != 0) return;

    // just pong
    enmod_inout_send_pong(timestamp);
}

void enmod_machine_recv_PONG(const uint8_t *src, const uint64_t timestamp)
{
    // check src == current peer
    if (memcmp(src, enmod_get_peer_mac(), ESP_NOW_ETH_ALEN) != 0) return;

    last_pingpong_timestamp_ms = timestamp;
}

/*
    RECEIVE CALLBACK
*/

void enmod_machine_recv_cb(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len)
{
    if (!data || data_len < 1) return;
    const uint8_t *src = esp_now_info->src_addr;
    msg_type_t msg_type = data[0];

    switch (msg_type) {
        /*
            discovering
        */
        case MSG_DISCOVERY:
            enmod_machine_recv_DISCOVERY(src, data);
            break;
        
        case MSG_DISCOVERY_ACK:
            enmod_machine_recv_DISCOVERY_ACK(src, data);
            break;
        
        /*
            pairing
        */
        case MSG_SECURE:
            enmod_machine_recv_SECURE(src, data);
            break;
        
        case MSG_SECURE_ACK:
            enmod_machine_recv_SECURE_ACK(src, data);
            break;
        
        /*
            reconnecting
        */
        case MSG_RECONNECT:
            enmod_machine_recv_RECONNECT(src, data);
            break;
        
        case MSG_RECONNECT_ACK:
            enmod_machine_recv_RECONNECT_ACK(src, data);
            break;
        
        /*
            ping-ponging
        */
        case MSG_PING:
            {
            uint64_t timestamp = 0;
            memcpy(&timestamp, data, sizeof(uint64_t));
            enmod_machine_recv_PING(src, timestamp);
            }
            break;
        
        case MSG_PONG:
            { 
            uint64_t timestamp = 0;
            memcpy(&timestamp, data, sizeof(uint64_t));
            enmod_machine_recv_PONG(src, timestamp);
            }
            break;
        
        /*
            misc
        */
        case MSG_TOGGLE:
            // todo
            break;
        
        case MSG_KBSTATE:
            // todo
            break;
    }
}

/*
    send cb
*/

void enmod_machine_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    ESP_LOGI(TAG, "Sent message to %02X:%02X:%02X:%02X:%02X:%02X",
        tx_info->des_addr[0], tx_info->des_addr[1], tx_info->des_addr[2],
        tx_info->des_addr[3], tx_info->des_addr[4], tx_info->des_addr[5]);
}

/*
    init
*/

void enmod_machine_init()
{
    esp_now_register_recv_cb(enmod_machine_recv_cb);
    esp_now_register_send_cb(enmod_machine_send_cb);

    xTaskCreate(
        enmod_machine_main_task,    // function
        "enmod_machine_main_task",  // name
        8192,                       // stack size
        NULL,                       // arg
        7,                          // priority
        NULL                        // handle
    );
}