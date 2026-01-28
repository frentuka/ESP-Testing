#pragma once

#include <stdint.h>
#include "tinyusb.h"

// ======== types ========

typedef enum usb_msg_type: uint8_t {
	CFG_GET = 0,
	CFG_SET,
	SYSTEM_GET,
    ACTION
} usb_msg_type_t;

// ======== flags ========

#define PAYLOAD_FLAG_FIRST 0b10000000 // 0x80
#define PAYLOAD_FLAG_MID   0b01000000 // 0x40
#define PAYLOAD_FLAG_LAST  0b00100000 // 0x20

// transport-wise
#define PAYLOAD_FLAG_ACK   0b00010000 // 0x10
#define PAYLOAD_FLAG_NAK   0b00001000 // 0x08

// process-wise
#define PAYLOAD_FLAG_OK    0b00000100 // 0x04
#define PAYLOAD_FLAG_ERR   0b00000010 // 0x02

#define PAYLOAD_FLAG_ABORT 0b00000001 // 0x01

// ======== single packet struct ========

#define MAX_PAYLOAD_LENGTH 43
typedef struct __attribute__ ((packed)) {
    uint8_t flags;
	uint16_t remaining_packets;
    uint8_t payload_len;
    uint8_t payload[MAX_PAYLOAD_LENGTH];
    uint8_t crt;
} usb_packet_msg_t;

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

void usb_callbacks_init(void);