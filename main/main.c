/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

//#include "wifi.h"
#include "button.h"
#include "rgb.h"
#include "enmod.h"

#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "usb_descriptors.h"
#include "usbmod.h"

#define TAG "MAIN"

enum ColorSet {
    Red, Green, Blue
};
enum ColorSet current_color = Red;

void send_char(char c)
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

void single_press_test()
{
    ESP_LOGI(TAG, "test single press");
    char* chars = "Tecleados";

    if (tud_mounted()) {
        while (*chars) {
            send_char(*chars);
            vTaskDelay(pdMS_TO_TICKS(50));
            chars++;
        }
    }

    // // ensure key release
    // uint8_t no_keys[6] = { 0 };
    // tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, no_keys);
}

void double_press_test()
{
    ESP_LOGI(TAG, "test double press");

    if (tud_mounted()) {
        uint8_t keycode[6] = { HID_KEY_GUI_LEFT };
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);  // Press
        vTaskDelay(pdMS_TO_TICKS(100));
        uint8_t no_keys[6] = { 0 };
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, no_keys); // Release
        ESP_LOGI(TAG, "Sent 'GUI'!");
    }
}

void on_espnow_state(espnow_state_t new_state) {
    switch (new_state) {
        case ESPNOW_STATE_IDLE:
            rgb_set_color((RGBColor){5, 5, 5});
            break;
        case ESPNOW_STATE_DISCOVERING:
            rgb_set_color((RGBColor){0, 0, 20});
            break;
        case ESPNOW_STATE_RECONNECTING:
            rgb_set_color((RGBColor){10, 20, 0});
            break;
        case ESPNOW_STATE_CONNECTED:
            rgb_set_color((RGBColor){0, 20, 0});
            break;
        case ESPNOW_STATE_SECURING:
            rgb_set_color((RGBColor){0, 30, 30});
            break;
        case ESPNOW_STATE_LINK_LOST:
            rgb_set_color((RGBColor){100, 30, 30});
            break;
        default:
            rgb_set_color((RGBColor){1, 2, 1});
            break;
    }
}


void app_main(void)
{
    printf("Hello world!!! :D\n");

    button_init(*single_press_test, *double_press_test);

    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.device = &desc_device;
    tusb_cfg.descriptor.full_speed_config = desc_configuration;
    tusb_cfg.descriptor.string = (const char **)string_desc_arr;
    tusb_cfg.descriptor.string_count = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]);

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // Start the background task for TinyUSB
    xTaskCreate(usb_task, "usb_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "USB initialized—plug into PC now!");
}