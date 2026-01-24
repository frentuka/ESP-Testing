#include "usb_callbacks.h"
#include "usb_descriptors.h"
#include "usb_crc.h"

#include "cfgmod.h"

#include "esp_log.h"

#include "tinyusb.h"

#define TAG "usb_callbacks"

// ============ HID CONFIG COMMS - Interrupt-driven Approach ============

// Buffers for config data
static uint8_t cfg_rx_buf[COMM_REPORT_SIZE];
static uint16_t cfg_rx_len = 0;

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
    
    // Validate payload
    if (payload_len == 0 || payload_len > sizeof(cfg_rx_buf)) {
        ESP_LOGE(TAG, "Invalid payload length: %d", payload_len);
        return;
    }

    // Process command
    uint8_t resp[COMM_REPORT_SIZE] = {0};
    size_t resp_len = 0;
    esp_err_t cfg_result = cfgmod_handle_usb_comm(payload, payload_len, resp, &resp_len, sizeof(resp));

    // Workaround: zero-fill and pad to COMM_REPORT_SIZE
    // Output needs to be exactly COMM_REPORT_SIZE bytes
    if (resp_len < COMM_REPORT_SIZE) {
        memset(resp + resp_len, 0, COMM_REPORT_SIZE - resp_len);
        resp_len = COMM_REPORT_SIZE;
    }

    if (cfg_result != ESP_OK) {
        ESP_LOGE(TAG, "cfgmod_handle_usb_comm() error: %s", esp_err_to_name(cfg_result));
        return;
    }

    if (!tud_mounted()) {
        ESP_LOGE(TAG, "TUD unmounted");
        return;
    }

    if (!tud_hid_n_ready(ITF_NUM_HID_COMM)) {
        ESP_LOGE(TAG, "ITF not ready");
        return;
    }

    // send
    bool usb_result = tud_hid_n_report(ITF_NUM_HID_COMM, REPORT_ID_COMM, resp, resp_len);

    if (!usb_result) {
        ESP_LOGE(TAG, "USB Send failed");
        return;
    }

    ESP_LOGI(TAG, "USB Send success");
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