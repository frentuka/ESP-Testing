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

#include "usbmod.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"

#include "kb_manager.h"

#define TAG "MAIN"

enum ColorSet {
    Red, Green, Blue
};
enum ColorSet current_color = Red;

void single_press_test()
{
    ESP_LOGI(TAG, "test single press");
    char* chars = "Tecleados";

    kb_manager_set_paused(true);
    vTaskDelay(pdMS_TO_TICKS(50));
    if (tud_mounted()) {
        while (*chars) {
            usb_send_char(*chars);
            vTaskDelay(pdMS_TO_TICKS(50));
            chars++;
        }
    }
    kb_manager_set_paused(false);

    // // ensure key release
    // uint8_t no_keys[6] = { 0 };
    // tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, no_keys);
}

void double_press_test()
{
    ESP_LOGI(TAG, "test double press");
    kb_manager_test_nkro_keypress(3, 3);
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
    
    usb_init();

    ESP_LOGI(TAG, "USB initialized—plug into PC now!");

    kb_manager_start();
}