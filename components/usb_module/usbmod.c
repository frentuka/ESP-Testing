#include <stdio.h>
#include <stdint.h>

#include "usbmod.h"

#include "esp_log.h"
#include "esp_err.h"

#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include "kb_manager.h"

#define TAG "USBModule"

// TinyUSB HID callbacks are required when HID is enabled in the descriptor/config.
// Provide minimal stubs to satisfy the linker (and optionally extend later).
uint16_t tud_hid_get_report_cb(uint8_t instance,
                              uint8_t report_id,
                              hid_report_type_t report_type,
                              uint8_t *buffer,
                              uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
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
}

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return desc_hid_report;
}

void usb_task(void *arg) {
    while (1) {
        tud_task();  // Run TinyUSB stack forever
        taskYIELD();
    }
}

/*
    ...
*/

void usb_send_char(char c)
{
    uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };
    uint8_t uichar = (uint8_t) c;
    if (uichar >= 128) return;

    uint8_t kc = conv_table[uichar][1];
    if (kc == 0) return;

    uint8_t mod = conv_table[uichar][0] ? KEYBOARD_MODIFIER_LEFTSHIFT : 0;
    uint8_t keys[6] = { kc };

    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, mod, keys);
    vTaskDelay(pdMS_TO_TICKS(20));
    uint8_t no_keys[6] = { 0 };
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, no_keys);
}

void usb_send_keystroke(uint8_t hid_keycode)
{
    if (tud_mounted()) {
        uint8_t keycode[6] = { hid_keycode };
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);  // Press
        vTaskDelay(pdMS_TO_TICKS(100));
        uint8_t no_keys[6] = { 0 };
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, no_keys); // Release
        ESP_LOGI(TAG, "Sent keystroke");
    }
}

/*
    kb reporting
*/

// True = Boot protocol (6KRO), False = Report protocol (NKRO)
bool usb_keyboard_use_boot_protocol(void)
{
    return (tud_hid_get_protocol() == HID_PROTOCOL_BOOT);
}

bool usb_send_keyboard_6kro(uint8_t modifier, const uint8_t keycodes[6])
{
    return tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycodes);
}

bool usb_send_keyboard_nkro(const uint8_t *bitmap, uint16_t len)
{
    return tud_hid_report(REPORT_ID_NKRO, bitmap, len);
}

/*
    init
*/

void usb_init()
{
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.device = &desc_device;
    tusb_cfg.descriptor.full_speed_config = desc_configuration;
    tusb_cfg.descriptor.string = (const char **)string_desc_arr;
    tusb_cfg.descriptor.string_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]);

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    xTaskCreatePinnedToCore(usb_task, "usb_task", 4096, NULL, 5, NULL, 0);
    ESP_LOGI(TAG, "Initialized!");
}