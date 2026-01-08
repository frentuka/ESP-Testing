// ESP-IDF 5.5.2 — Steps 1 and 2 (ESP32-S3)
// Build as a normal app_main() project.
// Wire:
//  Step 1: BTN pin -> momentary button -> GND
//  Step 2: COL pin -> switch -> ROW pin
//          (ROW uses internal pull-up; COL is driven HIGH/LOW)

#include <stdio.h>

#include "kb_matrix.h"

//#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"   // esp_rom_delay_us

// Change these GPIOs to whatever you wired.
#define BTN_GPIO   GPIO_NUM_4   // Step 1 button input (to GND)
#define COL_GPIO   GPIO_NUM_5   // Step 2 column output
#define ROW_GPIO   GPIO_NUM_6   // Step 2 row input (pull-up)

// ---------- Step 1: single button test (input + internal pull-up) ----------
static void step1_init_button(void) {
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << BTN_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
}

static void step1_run_button_demo(void) {
    int last = -1;
    while (1) {
        int level = gpio_get_level(BTN_GPIO); // 1 = released, 0 = pressed (to GND)
        if (level != last) {
            printf("[STEP 1] BTN=%d (%s)\n", level, level ? "released" : "pressed");
            last = level;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ---------- Step 2: 1x1 matrix test (drive COL, read ROW w/ pull-up) ----------
static void step2_init_1x1_matrix(void) {
    // Column output
    gpio_config_t col = {
        .pin_bit_mask = 1ULL << COL_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&col);
    gpio_set_level(COL_GPIO, 1); // idle HIGH

    // Row input with pull-up
    gpio_config_t row = {
        .pin_bit_mask = 1ULL << ROW_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&row);
}

static void step2_run_1x1_demo(void) {
    while (1) {
        // Case A: COL HIGH -> ROW should read HIGH regardless of switch
        gpio_set_level(COL_GPIO, 1);
        esp_rom_delay_us(5);
        int row_high_col = gpio_get_level(ROW_GPIO);

        // Case B: COL LOW -> if switch pressed, ROW should read LOW
        gpio_set_level(COL_GPIO, 0);
        esp_rom_delay_us(5);
        int row_low_col = gpio_get_level(ROW_GPIO);

        printf("[STEP 2] COL=1 => ROW=%d | COL=0 => ROW=%d (%s)\n",
               row_high_col,
               row_low_col,
               row_low_col ? "not pressed" : "pressed");

        // Restore idle
        gpio_set_level(COL_GPIO, 1);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}