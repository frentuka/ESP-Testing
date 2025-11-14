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

//#include "wifi.h"
#include "button.h"
#include "rgb.h"
#include "enmod.h"

#define TAG "MAIN"

enum ColorSet {
    Red, Green, Blue
};
enum ColorSet current_color = Red;

void single_press_test()
{
    ESP_LOGI(TAG, "test single press: %s", enmod_state_to_str(enmod_get_state()));
    
    if (enmod_get_state() == ESPNOW_STATE_PAIRED) enmod_send_toggle();
    else enmod_try_rejoin(5000);
}

void double_press_test()
{
    ESP_LOGI(TAG, "test double press");
    // switch (current_color) {
    //     case Red:
    //         rgb_set_color((RGBColor){20, 0, 0});
    //         current_color = Green;
    //         break;
    //     case Green:
    //         rgb_set_color((RGBColor){0, 20, 0});
    //         current_color = Blue;
    //         break;
    //     case Blue:
    //         rgb_set_color((RGBColor){0, 0, 20});
    //         current_color = Red;
    //         break;
    // }

    enmod_discovery_pair(5000);
}

void on_espnow_trigger() {
    rgb_toggle();
}

void on_espnow_state(espnow_state_t new_state) {
    switch (new_state) {
        case ESPNOW_STATE_NOT_PAIRED:
            rgb_set_color((RGBColor){20, 0, 0});
            break;
        case ESPNOW_STATE_PAIRING:
            rgb_set_color((RGBColor){0, 0, 20});
            break;
        case ESPNOW_STATE_PAIRED:
            rgb_set_color((RGBColor){0, 20, 0});
            break;
        case ESPNOW_STATE_REJOINING:
            rgb_set_color((RGBColor){30, 0, 35});
    }
}


void app_main(void)
{
    printf("Hello world!!! :D\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    //

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    //

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());


    //


    // if (wifi_init() != 0) {
    //     ESP_LOGI("MAIN", "wifi init failed");
    //     return;
    // }
    
    if (rgb_init(GPIO_NUM_48) != 0) {
        ESP_LOGI(TAG, "rgb_light_init failed");
        return;
    }

    rgb_set(true);
    rgb_set_color((RGBColor){10, 0, 0});
    vTaskDelay(pdMS_TO_TICKS(500));
    rgb_set_color((RGBColor){0, 10, 0});
    vTaskDelay(pdMS_TO_TICKS(500));
    rgb_set_color((RGBColor){0, 0, 10});
    vTaskDelay(pdMS_TO_TICKS(500));

    // inicializar espnow
    ESP_LOGI(TAG, "ESP INIT: %s", esp_err_to_name(enmod_init()));
    enmod_set_status_cb(on_espnow_state);
    enmod_set_toggle_cb(on_espnow_trigger);

    button_init(*single_press_test, *double_press_test);

    for (;;) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    // unreachable

    printf("Restarting now.\n");
    fflush(stdout);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    esp_restart();
}