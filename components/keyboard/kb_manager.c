#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "kb_manager.h"

#include "kb_matrix.h"
#include "kb_layout.h"

#include "usbmod.h"
#include "usb_descriptors.h"
#include "tusb.h" // for HID protocol enums if needed
#include "class/hid/hid.h"  // HID_KEY_* defines

static const char *TAG = "kb_manager";
static const uint8_t MIN_POLLING_RATE = 50;

static uint8_t s_debounce[KB_MATRIX_KEYS];              // per-key debounce integrator counters

#define KB_DEBOUNCE_SCANS 5

static volatile bool s_paused = false;
void kb_manager_set_paused(bool paused) { s_paused = paused; }

static inline void set_bit(uint8_t *bitmap, size_t bit_index)
{
    bitmap[bit_index >> 3] |= (uint8_t)(1U << (bit_index & 7U));
}

static inline void clear_bit(uint8_t *bitmap, size_t bit_index)
{
    bitmap[bit_index >> 3] &= (uint8_t)~(1U << (bit_index & 7U));
}

static inline bool get_bit(const uint8_t *bitmap, size_t bit_index)
{
    return (bitmap[bit_index >> 3] & (uint8_t)(1U << (bit_index & 7U))) != 0;
}

static void debounce_update(const uint8_t *raw, uint8_t *stable)
{
    for (size_t i = 0; i < KB_MATRIX_KEYS; ++i) {
        bool raw_pressed = get_bit(raw, i);

        if (raw_pressed) {
            if (s_debounce[i] < KB_DEBOUNCE_SCANS) {
                s_debounce[i]++;
            }
        } else {
            if (s_debounce[i] > 0) {
                s_debounce[i]--;
            }
        }

        if (s_debounce[i] == 0) {
            clear_bit(stable, i);
        } else if (s_debounce[i] >= KB_DEBOUNCE_SCANS) {
            set_bit(stable, i);
        }
    }
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

    uint32_t s_scan_count = 0;                           // scans accumulated for per-second benchmark
    int64_t s_last_benchmark_us = esp_timer_get_time();  // timestamp of last benchmark tick (microseconds)
    int64_t s_last_report_sent_us = esp_timer_get_time();// timestamp of last report. used to comply MIN_POLLING_RATE
    bool s_last_matrix_valid = false;                    // whether s_last_matrix contains valid data
    bool s_last_boot_protocol = false;                   // last USB HID protocol used (boot vs NKRO)
    uint32_t s_idle_yield_counter = 0;                   // counter for periodic idle yield

    uint8_t s_matrix[KB_MATRIX_BITMAP_BYTES];        // debounced/stable matrix bitmap (row-major)
    uint8_t s_raw_matrix[KB_MATRIX_BITMAP_BYTES];    // raw scan bitmap (row-major, no debounce)
    uint8_t s_nkro[NKRO_BYTES];                      // NKRO report bitmap (keycode-indexed)
    uint8_t s_last_matrix[KB_MATRIX_BITMAP_BYTES];   // last sent debounced matrix bitmap

    while (1) {
        scan(s_raw_matrix);
        debounce_update(s_raw_matrix, s_matrix);
        s_scan_count++;

        // benchmark
        int64_t now_us = esp_timer_get_time();
        int64_t elapsed_us = now_us - s_last_benchmark_us;
        if (elapsed_us >= 1000000) {
            uint32_t scans_per_sec = (uint32_t)((s_scan_count * 1000000LL) / elapsed_us);
            ESP_LOGI(TAG, "matrix scans/sec: %lu", (unsigned long)scans_per_sec);
            s_scan_count = 0;
            s_last_benchmark_us = now_us;
        }

        // prevent unnecessary usb reports
        bool boot_protocol = usb_keyboard_use_boot_protocol();
        bool matrix_changed = !s_last_matrix_valid ||
            (memcmp(s_matrix, s_last_matrix, KB_MATRIX_BITMAP_BYTES) != 0);

        bool should_send = tud_mounted() && !s_paused &&
            (matrix_changed || (boot_protocol != s_last_boot_protocol)
            || now_us > (s_last_report_sent_us + 1000000LL/MIN_POLLING_RATE)); // ensure min polling rate

        // send
        if (should_send) {
            s_last_report_sent_us = now_us;

            if (boot_protocol) {
                uint8_t keys[6];
                bitmap_to_6kro(s_matrix, keys);
                usb_send_keyboard_6kro(0, keys);

                //ESP_LOGI(TAG, "sent 6kro report");
            } else {
                matrix_to_nkro(s_matrix, s_nkro);
                usb_send_keyboard_nkro(s_nkro, NKRO_BYTES);

                //ESP_LOGI(TAG, "sent nkro report");
            }

            memcpy(s_last_matrix, s_matrix, KB_MATRIX_BITMAP_BYTES);
            s_last_matrix_valid = true;
            s_last_boot_protocol = boot_protocol;
        }

        // take 1 tick break every 64 scans (reduces rate from ~10k to 3.2k)
        if ((++s_idle_yield_counter & 0x3F) == 0) {
            vTaskDelay(1);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
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