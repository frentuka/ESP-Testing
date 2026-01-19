#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "kb_matrix.h"
#include "usbmod.h"
#include "tusb.h" // for HID protocol enums if needed
#include "hid.h"  // HID_KEY_* defines

// Optional weak keymap: override this in your app to map row/col -> HID keycode
__attribute__((weak)) uint8_t kb_keycode_from_rc(uint8_t row, uint8_t col)
{
    (void)row; (void)col;
    return HID_KEY_NONE;
}

static uint8_t s_matrix[KB_MATRIX_BITMAP_BYTES];

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
                uint8_t kc = kb_keycode_from_rc(r, c);
                if (kc != HID_KEY_NONE) {
                    keycodes[out++] = kc;
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

        if (usb_keyboard_use_boot_protocol()) {
            uint8_t keys[6];
            bitmap_to_6kro(s_matrix, keys);
            usb_send_keyboard_6kro(0, keys);
        } else {
            usb_send_keyboard_nkro(s_matrix, KB_MATRIX_BITMAP_BYTES);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void kb_manager_start(void)
{
    xTaskCreate(kb_manager_task, "kb_mgr", 4096, NULL, 5, NULL);
}