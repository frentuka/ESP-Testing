#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "usbmod.h"
#include "usb_descriptors.h"

#include "esp_log.h"
#include "esp_err.h"

#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tusb.h"

#include "cfgmod.h"

#define TAG "USBModule"

#define MAX_COMMS_SEND_ATTEMPTS 5

// ============ KEYBOARD HID - Keep unchanged ============

// HID Report callbacks are required for HID interfaces (keyboard)
uint16_t tud_hid_get_report_cb(uint8_t instance,
                              uint8_t report_id,
                              hid_report_type_t report_type,
                              uint8_t *buffer,
                              uint16_t reqlen)
{
    (void) instance;
    (void) report_type;
    (void) report_id;
    (void) buffer;
    
    // Keyboard doesn't need to respond to get_report
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t bufsize)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
    
    // Keyboard doesn't process incoming data
}

bool usb_keyboard_use_boot_protocol(void)
{
    return (tud_hid_n_get_protocol(ITF_NUM_HID_KBD) == HID_PROTOCOL_BOOT);
}

bool usb_send_keyboard_6kro(uint8_t modifier, const uint8_t keycodes[6])
{
    return tud_hid_n_keyboard_report(ITF_NUM_HID_KBD, REPORT_ID_KEYBOARD, modifier, keycodes);
}

bool usb_send_keyboard_nkro(const uint8_t *bitmap, uint16_t len)
{
    return tud_hid_n_report(ITF_NUM_HID_KBD, REPORT_ID_NKRO, bitmap, len);
}

void usb_send_char(char c)
{
    uint8_t const conv_table[128][2] = { HID_ASCII_TO_KEYCODE };
    uint8_t uichar = (uint8_t) c;
    if (uichar >= 128) return;

    uint8_t kc = conv_table[uichar][1];
    if (kc == 0) return;

    uint8_t mod = conv_table[uichar][0] ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
    uint8_t keys[6] = { kc };

    tud_hid_n_keyboard_report(ITF_NUM_HID_KBD, REPORT_ID_KEYBOARD, mod, keys);
    vTaskDelay(pdMS_TO_TICKS(20));
    uint8_t no_keys[6] = { 0 };
    tud_hid_n_keyboard_report(ITF_NUM_HID_KBD, REPORT_ID_KEYBOARD, 0, no_keys);
}

void usb_send_keystroke(uint8_t hid_keycode)
{
    if (tud_mounted()) {
        uint8_t keycode[6] = { hid_keycode };
        tud_hid_n_keyboard_report(ITF_NUM_HID_KBD, REPORT_ID_KEYBOARD, 0, keycode);  // Press
        vTaskDelay(pdMS_TO_TICKS(100));
        uint8_t no_keys[6] = { 0 };
        tud_hid_n_keyboard_report(ITF_NUM_HID_KBD, REPORT_ID_KEYBOARD, 0, no_keys); // Release
        ESP_LOGI(TAG, "Sent keystroke");
    }
}

// ============ BULK COMM - New Implementation ============

// Buffers for bulk endpoint data
static uint8_t bulk_rx_buf[BULK_COMM_MAX_SIZE];
static uint16_t bulk_rx_len = 0;
static uint8_t bulk_tx_buf[BULK_COMM_MAX_SIZE];

// Called whenever there's data available on the bulk OUT endpoint (host -> device)
void tud_vendor_rx_cb(uint8_t itf, uint8_t const *buffer, uint16_t bufsize)
{
    if (itf != ITF_NUM_BULK_COMM || buffer == NULL || bufsize == 0) {
        return;
    }

    // Copy received data to buffer
    uint16_t copy_len = (bufsize < sizeof(bulk_rx_buf)) ? bufsize : sizeof(bulk_rx_buf);
    memcpy(bulk_rx_buf, buffer, copy_len);
    bulk_rx_len = copy_len;

    ESP_LOGI(TAG, "Bulk RX: %d bytes", bulk_rx_len);

    // Process the command and prepare response
    uint8_t resp[BULK_COMM_MAX_SIZE] = {0};
    size_t resp_len = 0;

    esp_err_t handle_result = cfgmod_handle_usb_comm(bulk_rx_buf, bulk_rx_len, resp, &resp_len, sizeof(resp));
    if (handle_result != ESP_OK || resp_len == 0) {
        ESP_LOGE(TAG, "cfgmod_handle_usb_comm() error: %s, resp_len: %d", 
                 esp_err_to_name(handle_result), resp_len);
        return;
    }

    // Send response
    if (!usb_comm_send(resp, resp_len)) {
        ESP_LOGE(TAG, "Failed to send response");
    }
}

// Send data via bulk IN endpoint (device -> host)
bool usb_comm_send(const uint8_t *data, uint16_t len)
{
    if (len > BULK_COMM_MAX_SIZE) len = BULK_COMM_MAX_SIZE;
    
    memcpy(bulk_tx_buf, data, len);
    
    // tud_vendor_write blocks until data is sent or endpoint is full
    uint16_t written = tud_vendor_write(bulk_tx_buf, len);
    
    ESP_LOGI(TAG, "Bulk TX: %d/%d bytes written", written, len);
    return (written == len);
}

// Read data from bulk endpoint (typically called from cfgmod or main app)
uint16_t usb_comm_read(uint8_t *out, uint16_t max_len)
{
    uint16_t n = bulk_rx_len;
    if (n > max_len) n = max_len;
    memcpy(out, bulk_rx_buf, n);
    bulk_rx_len = 0;  // Clear after read
    return n;
}

// HID report descriptor callback for keyboard
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    // Only keyboard HID uses this now
    return desc_hid_report_kbd;
}

// TinyUSB task
void usb_task(void *arg) {
    while (1) {
        tud_task();
        taskYIELD();
    }
}

void usb_init()
{
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.device = &desc_device;
    tusb_cfg.descriptor.full_speed_config = desc_configuration;
    tusb_cfg.descriptor.string = (const char **)string_desc_arr;
    tusb_cfg.descriptor.string_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]);

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    xTaskCreatePinnedToCore(usb_task, "usb_task", 4096, NULL, 5, NULL, 1);
    ESP_LOGI(TAG, "USB initialized with HID Keyboard + Bulk COMM!");
}