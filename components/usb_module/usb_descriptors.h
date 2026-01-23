#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include "tusb.h"

// ------------ Device Descriptor ------------
static tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x303A,
    .idProduct          = 0x1324,
    .bcdDevice          = 0x0101,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// ------------ HID Report Descriptor (Keyboard Only) ------------
#define REPORT_ID_KEYBOARD 1
#define REPORT_ID_NKRO 2

#define NKRO_KEYS 0xE7
#define NKRO_BYTES ((NKRO_KEYS + 7) / 8)
#define NKRO_REPORT_SIZE 64

static uint8_t const desc_hid_report_kbd[] = {
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
    0x75, 0x01,                         // Report Size (1)
    0x95, (NKRO_BYTES*8 - NKRO_KEYS),   // Report Count (padding)
    0x81, 0x03,                         // Input (Const,Var,Abs)
    0xC0                                // End Collection
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance);

// ------------ Interface & Endpoint Definitions ------------
#define ITF_NUM_HID_KBD   0
#define ITF_NUM_BULK_COMM 1
#define ITF_NUM_TOTAL     2

#define EPNUM_HID_KBD_IN  0x81
#define EPNUM_BULK_OUT    0x02
#define EPNUM_BULK_IN     0x82

#define BULK_COMM_MAX_SIZE 64

// ------------ Configuration Descriptor ------------
// Use TinyUSB macros for keyboard HID interface (proper format)
// Manual descriptor for bulk interface to avoid HID limitations

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + 9 + 7 + 7)

static uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(
        1,                              // bConfigurationValue
        ITF_NUM_TOTAL,                  // bNumInterfaces (2)
        0,                              // iConfiguration
        CONFIG_TOTAL_LEN,               // wTotalLength
        0x80,                           // bmAttributes (bus powered)
        100                             // bMaxPower (200 mA)
    ),

    // Interface 0: HID Keyboard (using TinyUSB macro)
    TUD_HID_DESCRIPTOR(
        ITF_NUM_HID_KBD,                // bInterfaceNumber
        4,                              // iInterface (string index)
        true,                           // protocol (boot)
        sizeof(desc_hid_report_kbd),    // wDescriptorLength
        EPNUM_HID_KBD_IN,               // bEndpointAddress
        NKRO_REPORT_SIZE,               // wMaxPacketSize
        1                               // bInterval
    ),

    // Interface 1: Bulk COMM (manual - vendor-specific, not HID)
    // Interface descriptor (9 bytes)
    9,                                  // bLength
    TUSB_DESC_INTERFACE,               // bDescriptorType
    ITF_NUM_BULK_COMM,                 // bInterfaceNumber
    0,                                  // bAlternateSetting
    2,                                  // bNumEndpoints (OUT + IN)
    TUSB_CLASS_VENDOR_SPECIFIC,        // bInterfaceClass (vendor-specific)
    0,                                  // bInterfaceSubClass
    0,                                  // bInterfaceProtocol
    5,                                  // iInterface (string index)

    // Endpoint OUT (7 bytes)
    7,                                  // bLength
    TUSB_DESC_ENDPOINT,                // bDescriptorType
    EPNUM_BULK_OUT,                    // bEndpointAddress
    TUSB_XFER_BULK,                    // bmAttributes (bulk)
    U16_TO_U8S_LE(BULK_COMM_MAX_SIZE), // wMaxPacketSize
    0,                                  // bInterval

    // Endpoint IN (7 bytes)
    7,                                  // bLength
    TUSB_DESC_ENDPOINT,                // bDescriptorType
    EPNUM_BULK_IN,                     // bEndpointAddress
    TUSB_XFER_BULK,                    // bmAttributes (bulk)
    U16_TO_U8S_LE(BULK_COMM_MAX_SIZE), // wMaxPacketSize
    0                                   // bInterval
};

// ------------ String Descriptors ------------
static char const* string_desc_arr[] = {
    (const char[]){0x09, 0x04},         // 0: Language
    "Tecleados",                        // 1: Manufacturer
    "DF-ONE",                           // 2: Product
    "13548",                            // 3: Serial
    "Tecleados Deltafors Donaltron",    // 4: Keyboard interface
    "Tecleados Comms ITF"               // 5: Bulk COMM interface
};

#endif /* USB_DESCRIPTORS_H_ */