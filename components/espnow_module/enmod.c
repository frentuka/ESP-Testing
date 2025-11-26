#include <stdio.h>

#include "enmod.h"
#include "enmod_basics.h"
#include "enmod_inout.h"
#include "enmod_machine.h"

#include "esp_mac.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "esp_timer.h"
#include "esp_log.h"
#include "esp_chip_info.h"

#include "esp_rom_sys.h"
#include "freertos/semphr.h"

#define TAG "ESPNOW_MAIN"

static bool init = false;

/*
    getters
*/

const bool enmod_is_init(void)
{
    return init;
}

/*
    inits
*/

static esp_err_t nvs_safe_init(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs to be erased (err=0x%x). Re-initializing...", err);
        // Erase and re-init
        esp_err_t erase_err = nvs_flash_erase();
        if (erase_err == ESP_OK) {
            err = nvs_flash_init();
        } else {
            ESP_LOGE(TAG, "Failed to erase NVS (err=0x%x)", erase_err);
            return err; // Do not panic, just return
        }
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS (err=0x%x)", err);
        return err; // Fail silently
    }

    ESP_LOGI(TAG, "NVS ready");
    return err;
}

static esp_err_t wifi_sta_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
    return ESP_OK;
}

esp_err_t enmod_init(void)
{
    ESP_ERROR_CHECK(nvs_safe_init());
    ESP_ERROR_CHECK(wifi_sta_init());
    ESP_ERROR_CHECK(esp_now_init());

    enmod_machine_init();

    init = true;
    return ESP_OK;
}