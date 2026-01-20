#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include "tusb.h"

// ------------ Device Descriptor ------------
// Tells the PC basic info about your device
static tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,        // USB 2.0
    .bDeviceClass       = 0x00,          // Specified in interface descriptor
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x303A,        // Espressif's VID (safe for testing)
    .idProduct          = 0x1324,        // Arbitrary PID
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// ------------ HID Report Descriptor ------------
// Defines the keyboard report format (standard boot keyboard)
#define REPORT_ID_KEYBOARD 1
#define REPORT_ID_NKRO 2

#define NKRO_KEYS 0xE7
#define NKRO_BYTES ((NKRO_KEYS + 7) / 8)

// Boot keyboard (6KRO) + NKRO bitmap
static uint8_t const desc_hid_report[] = {
    // 6KRO boot keyboard
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),

    // NKRO bitmap
    0x05, 0x01,                         // Usage Page (Generic Desktop)
    0x09, 0x06,                         // Usage (Keyboard)
    0xA1, 0x01,                         // Collection (Application)
    0x85, REPORT_ID_NKRO,               // Report ID
    0x05, 0x07,                         // Usage Page (Keyboard/Keypad)
    0x19, 0x00,                         // Usage Minimum (0)
    0x29, (NKRO_KEYS - 1),              // Usage Maximum
    0x15, 0x00,                         // Logical Minimum (0)
    0x25, 0x01,                         // Logical Maximum (1)
    0x75, 0x01,                         // Report Size (1)
    0x95, NKRO_KEYS,                    // Report Count
    0x81, 0x02,                         // Input (Data,Var,Abs)
                                        // padding to byte boundary
    0x75, 0x01,                         // Report Size (1)
    0x95, (NKRO_BYTES*8 - NKRO_KEYS),   // Report Count (padding)
    0x81, 0x03,                         // Input (Const,Var,Abs)
    0xC0                                // End Collection
};

// Callback: PC asks for HID report descriptor
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance);

// ------------ Configuration Descriptor ------------
// Combines config + HID interface + endpoint
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

#define EPNUM_HID_IN 0x81  // Interrupt IN endpoint

static uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, 0x00, 100),  // Config
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(desc_hid_report), EPNUM_HID_IN, 8, 10)  // HID interface (polling 10ms)
};

// ------------ String Descriptors ------------
// Human-readable strings (manufacturer, product, etc.)
static char const* string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: English (US)
    "Tecleados",                // 1: Manufacturer
    "DF-ONE",                   // 2: Product
    "13548",                    // 3: Serial
    "Tecleados Deltafors Donaltron"
};

#endif /* USB_DESCRIPTORS_H_ */