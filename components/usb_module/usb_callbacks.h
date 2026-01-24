#pragma once

#include <stdint.h>
#include "tinyusb.h"

// ======== types ========

typedef enum usb_cmd_msg_type: uint8_t {
	CMD_MSG_REQUEST_SET = 0,
    CMD_MSG_REQUEST_GET,
	CMD_MSG_RESPONSE,
	CMD_MSG_NOTIFY
} usb_cmd_msg_type_t;

typedef enum usb_cmd_msg_kind: uint8_t {
	CMD_KIND_LAYOUT = 0,
	CMD_KIND_MACRO,
    CMD_KIND_CONNECTION,
	CMD_KIND_SYSTEM
} usb_cmd_msg_kind_t;

// ======== messages ========

#define FLAG_FIRST 0b10000000
#define FLAG_LAST  0b00000001
#define FLAG_ACK   0b01000000
#define FLAG_ABORT 0b00100000

typedef struct cmd_msg {
	usb_cmd_msg_type_t type;
	usb_cmd_msg_kind_t kind;
    uint8_t flags;
	uint16_t seq;
	uint16_t payload_len;
    uint8_t payload[40];
} cmd_msg_t;

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