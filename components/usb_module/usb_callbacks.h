#pragma once

#include <stdint.h>
#include "tinyusb.h"

#define ACK_KEY = 0b10101010
#define NAK_KEY = 0b01010101
#define OK_KEY  = 0b11110000
#define ERR_KEY = 0b00001111

typedef struct {
    uint8_t  cmd;           // Command ID (0x01 = set, 0x02 = get, 0x03 = action)
    uint16_t total_len;     // Total bytes of the full payload (little-endian)
    uint8_t  key_id;        // Which config key/slot this data belongs to (0-255)
    uint8_t  flags;         // Bitfield: (b0=first packet, b1=last packet)

    uint8_t reserved[2];    // one never knows

    uint8_t  payload[40];   // Actual data bytes (40 to reach exactly 47 before CRC)

    // CRC is NOT inside the struct but appended right after
} __attribute__((packed)) command_payload_t;

typedef struct {
    uint8_t  cmd;           // Command ID (0x01 = set, 0x02 = get, 0x03 = action)
    uint16_t total_len;     // Total bytes of the full payload (little-endian)
    uint8_t  key_id;        // Which config key/slot this data belongs to (0-255)
    uint8_t  flags;         // Bitfield: (b0=first packet, b1=last packet)

    uint8_t reserved[2];    // one never knows

    uint8_t  payload[40];   // Actual data bytes (40 to reach exactly 47 before CRC)

    // CRC is NOT inside the struct but appended right after
} __attribute__((packed)) command_rsp_payload_t;

// TinyUSB HID callbacks are required when HID is enabled in the descriptor/config.
// Provide minimal stubs to satisfy the linker (and optionally extend later).
uint16_t usbmod_tud_hid_get_report_cb(uint8_t instance,
                              uint8_t report_id,
                              hid_report_type_t report_type,
                              uint8_t *buffer,
                              uint16_t reqlen);

void usbmod_tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t bufsize);