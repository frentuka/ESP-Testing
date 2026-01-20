#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "kb_manager.h"

#include "kb_matrix.h"
#include "kb_layout.h"

#include "usbmod.h"
#include "usb_descriptors.h"
#include "tusb.h" // for HID protocol enums if needed
#include "class/hid/hid.h"  // HID_KEY_* defines

static uint8_t s_matrix[KB_MATRIX_BITMAP_BYTES];
static uint8_t s_nkro[NKRO_BYTES];

static volatile bool s_paused = false;
void kb_manager_set_paused(bool paused) { s_paused = paused; }

static inline void set_bit(uint8_t *bitmap, size_t bit_index)
{
    bitmap[bit_index >> 3] |= (uint8_t)(1U << (bit_index & 7U));
}

static void bitmap_to_6kro(const uint8_t *bitmap, uint8_t keycodes[6])
{
    memset(keycodes, 0, 6);
    size_t out = 0;

    for (uint8_t r = 0; r < KB_MATRIX_ROW_COUNT && out < 6; ++r) {
        for (uint8_t c = 0; c < KB_MATRIX_COL_COUNT && out < 6; ++c) {
            size_t bit_index = (r * KB_MATRIX_COL_COUNT) + c;
            uint8_t byte = bitmap[bit_index >> 3];
            uint8_t bit = (uint8_t)(1U << (bit_index & 7U));
            if (byte & bit) {
                uint8_t kc = kb_layout_get_keycode(r, c);
                if (kc != HID_KEY_NONE) {
                    keycodes[out++] = kc;
                }
            }
        }
    }
}

static void matrix_to_nkro(const uint8_t *matrix, uint8_t *nkro)
{
    memset(nkro, 0, NKRO_BYTES);

    for (uint8_t r = 0; r < KB_MATRIX_ROW_COUNT; ++r) {
        for (uint8_t c = 0; c < KB_MATRIX_COL_COUNT; ++c) {
            size_t bit_index = (r * KB_MATRIX_COL_COUNT) + c;
            uint8_t byte = matrix[bit_index >> 3];
            uint8_t bit = (uint8_t)(1U << (bit_index & 7U));
            if (byte & bit) {
                uint8_t kc = kb_layout_get_keycode(r, c);
                if (kc != HID_KEY_NONE && kc < NKRO_KEYS) {
                    set_bit(nkro, kc);
                }
            }
        }
    }
}

static void kb_manager_task(void *arg)
{
    (void)arg;

    while (1) {
        scan(s_matrix);

        if (tud_mounted() && !s_paused) {
            if (usb_keyboard_use_boot_protocol()) {
                uint8_t keys[6];
                bitmap_to_6kro(s_matrix, keys);
                usb_send_keyboard_6kro(0, keys);
            } else {
                matrix_to_nkro(s_matrix, s_nkro);
                usb_send_keyboard_nkro(s_nkro, NKRO_BYTES);
            }
        }

        taskYIELD();
    }
}

void kb_manager_start(void)
{
    kb_matrix_gpio_init();
    vTaskDelay(pdMS_TO_TICKS(500));
    xTaskCreatePinnedToCore(kb_manager_task, "kb_mgr", 4096, NULL, 5, NULL, 1);
}

void kb_manager_test_nkro_keypress(uint8_t row, uint8_t col)
{
    uint8_t kc = kb_layout_get_keycode(row, col);
    if (kc == HID_KEY_NONE || kc >= NKRO_KEYS) {
        return;
    }

    uint8_t nkro[NKRO_BYTES];
    memset(nkro, 0, sizeof(nkro));
    nkro[kc >> 3] |= (uint8_t)(1U << (kc & 7U));

    usb_send_keyboard_nkro(nkro, sizeof(nkro));
    vTaskDelay(pdMS_TO_TICKS(20));

    memset(nkro, 0, sizeof(nkro));
    usb_send_keyboard_nkro(nkro, sizeof(nkro));
}