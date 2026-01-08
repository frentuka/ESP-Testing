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

static uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD))
};

// Callback: PC asks for HID report descriptor
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return desc_hid_report;
}

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