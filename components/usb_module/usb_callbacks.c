#include "usb_callbacks.h"
#include "usb_descriptors.h"
#include "usb_crc.h"

#include "cfgmod.h"
#include "basic_utils.h"

#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "tinyusb.h"

#define TAG "usb_callbacks"

#define MAX_RX_BUF_SIZE 21500 // 21.5KB -> 43(payload size) * 100(polling rate) * 5(seconds)
#define MAX_RX_BUF_SIZE_IN_PAYLOADS MAX_RX_BUF_SIZE / MAX_PAYLOAD_LENGTH // 500 packets long
// which is the max amount of packets that can ideally be sent in 5 seconds

// function declaration here for wide usage, implementation at the bottom
static void erase_rx_buffer();
static bool append_to_rx_buffer(const uint8_t *data, uint8_t data_len);
static bool process_rx_buffer();

static void process_tx_callback(const uint8_t *payload);
static void process_rx_callback(const uint8_t *payload);

static void process_recvd_flag_ack();
static void process_recvd_flag_nak();
static void process_recvd_flag_ok();
static void process_recvd_flag_err();
static void process_recvd_flag_abort();

static void send_ack(uint8_t idx);
static void send_nak(uint8_t idx);
static void send_ok(uint8_t idx);
static void send_err(uint8_t idx);
static void send_abort(void);

// ============ HID CONFIG COMMS - Interrupt-driven Approach ============

// ======== Packet processing queues ========
#define PROCESS_QUEUE_LENGTH 4
static QueueHandle_t process_queue = NULL;

// Buffers to capture full payload before executing order
static uint8_t rx_buf[MAX_RX_BUF_SIZE] = {0};
static uint16_t rx_len = 0;
static uint16_t last_packet_remaining_packets = 0;

// Updated HID callbacks to route by interface
uint16_t usbmod_tud_hid_get_report_cb(uint8_t instance,
                              uint8_t report_id,
                              hid_report_type_t report_type,
                              uint8_t *buffer,
                              uint16_t reqlen)
{
    (void) report_type;
    (void) report_id;
    
    // Nothing is done here since all requests are interrupt based
    return 0;
}

void usbmod_tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t bufsize)
{
    (void) report_type;
    
    if (instance != ITF_NUM_HID_COMM) {
        return;
    }

    // Skip the report ID byte - buffer[0] is the report ID, actual data starts at buffer[1]
    // bufsize includes the report ID, so actual payload is (bufsize - 1)
    uint16_t payload_len = bufsize > 0 ? bufsize - 1 : 0;
    uint8_t const *payload = buffer + 1;

    ESP_LOGI(TAG, "HID Comms RX: %d bytes (payload only)", payload_len);
    
    // Validate payload availability (only full sized messages are allowed)
    if (payload_len == 0 || payload_len > sizeof(usb_packet_msg_t)) {
        ESP_LOGE(TAG, "Invalid payload length: %d", payload_len);
        return;
    }

    // Validate payload's CRT
    if (!usb_crc_verify_packet(payload)) {
        ESP_LOGE(TAG, "Unable to validate payload. Responding with ERR.");
        //return; // to-do: answer with ERR
    }

    // Process packet
    usb_packet_msg_t msg = {0};
    memcpy(&msg, payload, payload_len);

    // Send packet to be processed on another thread
    if (process_queue == NULL) {
        ESP_LOGE(TAG, "Process queue not initialized");
        return;
    }

    if (xQueueSend(process_queue, &msg, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Process queue full, dropping packet");
    }
}

static void process_rx_packet(usb_packet_msg_t msg)
{
    ESP_LOGI(TAG, "Received payload. len: %u, flags: %u, remaining: %u", msg.payload_len, msg.flags, msg.remaining_packets);
    print_bytes_as_chars(TAG, msg.payload, msg.payload_len);

    // Verify payload's length
    if (msg.payload_len == 0) {
        ESP_LOGE(TAG, "Received payload_len == 0");
        return;
    }

    // Verify remaining_packets
    if (msg.remaining_packets >= last_packet_remaining_packets) {
        erase_rx_buffer();
        last_packet_remaining_packets = msg.remaining_packets;
    }

    // Process flags
    bool result = false;
    switch (msg.flags)
    {
        // rx-wise

        case PAYLOAD_FLAG_FIRST: // being the first one, erase buffer and start storing it
            erase_rx_buffer();
            result = append_to_rx_buffer(msg.payload, msg.payload_len);
            if (!result) {
                ESP_LOGE(TAG, "Error when appending to rx buffer");
                send_nak(0);
            }
            break;

        case PAYLOAD_FLAG_MID:
            result = append_to_rx_buffer(msg.payload, msg.payload_len);
            if (!result) {
                ESP_LOGE(TAG, "Error when appending to rx buffer");
                send_nak(msg.remaining_packets);
            }
            break;
        
        case PAYLOAD_FLAG_LAST: // being the last one, append and send buffer to be processed
            append_to_rx_buffer(msg.payload, msg.payload_len);
            result = process_rx_buffer();
            if (!result) {
                ESP_LOGE(TAG, "Error when processing rx buffer");
                erase_rx_buffer();
                send_nak(msg.remaining_packets);
            }
            break;
        
        // tx-wise (todo)

        case PAYLOAD_FLAG_ACK:
            process_recvd_flag_ack();
            break;

        case PAYLOAD_FLAG_NAK:
            process_recvd_flag_nak();
            break;
        
        case PAYLOAD_FLAG_OK:
            process_recvd_flag_ok();
            break;

        case PAYLOAD_FLAG_ERR:
            process_recvd_flag_err();
            break;
        
        case PAYLOAD_FLAG_ABORT:
            process_recvd_flag_abort();
            break;

        default:
            ESP_LOGE(TAG, "Unable to determine payload flag for %u", msg.flags);
            break;
    }
}

// HID report descriptor callback - return correct descriptor per interface
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    if (instance == ITF_NUM_HID_KBD) {
        return desc_hid_report_kbd;
    } else if (instance == ITF_NUM_HID_COMM) {
        return desc_hid_report_comm;
    }
    return NULL;
}

// ======== rx processing ========

// Erase rx buffer in order to receive new data
static void erase_rx_buffer()
{
    memset(rx_buf, 0, sizeof(rx_buf));
    rx_len = 0;
    ESP_LOGI(TAG, "RX buffer erased");
}

// Will store data in rx buffer appending it next to data already stored
static bool append_to_rx_buffer(const uint8_t *data, uint8_t data_len)
{
    if (rx_len + data_len > MAX_RX_BUF_SIZE) {
        ESP_LOGE(TAG, "Error: trying to append packets bigger than available space (trying: %lu, max av: %lu)", data_len + rx_len, MAX_RX_BUF_SIZE);
        return false;
    }

    memcpy(rx_buf + rx_len, data, data_len);
    rx_len+= data_len;

    ESP_LOGI(TAG, "Appended data. Data length: %d. Buffer length: %d sizeof(rx_buf): %ull", data_len, rx_len, sizeof(rx_buf));

    return true;
}

// This function should be called when all rx_buffer contains a full set of good payloads
static bool process_rx_buffer()
{
    ESP_LOGI(TAG, "Processing RX buffer. rx_len: %u", rx_len);
    print_bytes_as_chars(TAG, rx_buf, rx_len);
    return true;
}

// ======== flag processing ========

static void process_recvd_flag_ack()
{
    // todo
}

static void process_recvd_flag_nak()
{
    // todo
}

static void process_recvd_flag_ok()
{
    // todo
}

static void process_recvd_flag_err()
{
    // todo
}

static void process_recvd_flag_abort()
{
    // todo
}

// ======== responses ========

#define PACKET_SEND_MAX_ATTEMPTS 3
#define PACKET_SEND_ATTEMPT_DELAY_MS 10

// will send a packet of length COMM_REPORT_SIZE - 1 where last byte will be CRC
static void send_single_packet(uint8_t *packet, uint16_t packet_len)
{
    // fix packet length
    if (packet_len < COMM_REPORT_SIZE) {
        memset(packet + packet_len, 0, COMM_REPORT_SIZE - packet_len);
        packet_len = COMM_REPORT_SIZE;
    }

    usb_crc_prepare_packet(packet);
    bool result = tud_hid_n_report(ITF_NUM_HID_COMM, REPORT_ID_COMM, packet, packet_len);
    for (uint8_t i = 1; !result && 1 < PACKET_SEND_MAX_ATTEMPTS; i++) {
        ESP_LOGE(TAG, "Failed report send attempt...");
        vTaskDelay(pdMS_TO_TICKS(PACKET_SEND_ATTEMPT_DELAY_MS));
        result = tud_hid_n_report(ITF_NUM_HID_COMM, REPORT_ID_COMM, packet, packet_len);
    }

    if (result) {
        ESP_LOGI(TAG, "Report sent successfully");
    }
}

static void send_ack(uint8_t idx)
{

}

static void send_nak(uint8_t idx)
{

}

static void send_ok(uint8_t idx)
{

}

static void send_err(uint8_t idx)
{

}

static void send_abort(void)
{

}

// ======== init ========

static void processing_task(void *pvParameters) {
    usb_packet_msg_t msg;
    while (1) {
        if (xQueueReceive(process_queue, &msg, portMAX_DELAY) == pdTRUE) {
            process_rx_packet(msg);
        }
    }
}

void usb_callbacks_init(void) {
    process_queue = xQueueCreate(PROCESS_QUEUE_LENGTH, sizeof(usb_packet_msg_t));
    if (process_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create process_queue");
        return;
    }
    xTaskCreate(processing_task, "usb_processing_task", 4096, NULL, 5, NULL);
}