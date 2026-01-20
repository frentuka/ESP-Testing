// ESP-IDF 5.5.2 — Steps 1 and 2 (ESP32-S3)
// Build as a normal app_main() project.
// Wire:
//  Step 1: BTN pin -> momentary button -> GND
//  Step 2: COL pin -> switch -> ROW pin
//          (ROW uses internal pull-up; COL is driven HIGH/LOW)

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "kb_matrix.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"

typedef struct {
	uint8_t index;
	gpio_num_t gpio;
} kb_gpio_t;

static const kb_gpio_t k_rows[] = {
	GPIO_ROWS
};

static const kb_gpio_t k_cols[] = {
	GPIO_COLS
};

static uint64_t kb_build_pin_mask(const kb_gpio_t *pins, size_t count) {
	uint64_t mask = 0;
	for (size_t i = 0; i < count; ++i) {
		mask |= (1ULL << pins[i].gpio);
	}
	return mask;
}

/*
    scanning
*/
static inline void kb_set_bit(uint8_t *bitmap, size_t bit_index) {
	bitmap[bit_index >> 3] |= (uint8_t)(1U << (bit_index & 7U));
}

void scan(uint8_t *out_matrix_bitmap) {
	const size_t row_count = sizeof(k_rows) / sizeof(k_rows[0]);
	const size_t col_count = sizeof(k_cols) / sizeof(k_cols[0]);
	const size_t total_bits = row_count * col_count;
	const size_t total_bytes = (total_bits + 7U) / 8U;

	memset(out_matrix_bitmap, 0, total_bytes);

	for (size_t c = 0; c < col_count; ++c) {
		gpio_set_level(k_cols[c].gpio, 0);
		esp_rom_delay_us(5);

		for (size_t r = 0; r < row_count; ++r) {
			int level = gpio_get_level(k_rows[r].gpio);
			if (level == 0) {
				size_t bit_index = (k_rows[r].index * col_count) + k_cols[c].index;
				kb_set_bit(out_matrix_bitmap, bit_index);
			}
		}

		gpio_set_level(k_cols[c].gpio, 1);
	}
}

/*
    init
*/
void kb_matrix_gpio_init(void) {
	const size_t row_count = sizeof(k_rows) / sizeof(k_rows[0]);
	const size_t col_count = sizeof(k_cols) / sizeof(k_cols[0]);

	gpio_config_t row_cfg = {
		.pin_bit_mask = kb_build_pin_mask(k_rows, row_count),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&row_cfg);

	gpio_config_t col_cfg = {
		.pin_bit_mask = kb_build_pin_mask(k_cols, col_count),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&col_cfg);

	for (size_t i = 0; i < col_count; ++i) {
		gpio_set_level(k_cols[i].gpio, 1);
	}
}