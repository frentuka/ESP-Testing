#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_log.h"

const uint16_t PRINT_BYTES_AS_CHARS_MAX = 128;

static inline void print_bytes_as_chars(const char *tag, const uint8_t *data, size_t len) {
    size_t offset = 0;
    while (offset < len) {
        size_t chunk = (len - offset > PRINT_BYTES_AS_CHARS_MAX - 1) ? PRINT_BYTES_AS_CHARS_MAX - 1 : len - offset;
        char buf[PRINT_BYTES_AS_CHARS_MAX];
        for (size_t i = 0; i < chunk; ++i) {
            buf[i] = (char)data[offset + i];
        }
        buf[chunk] = '\0';
        ESP_LOGI(tag, "Chars: %s", buf);
        offset += chunk;
    }
}