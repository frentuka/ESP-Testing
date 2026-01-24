idf_component_register(
    SRCS "usb_callbacks.c" "usbmod.c"
    INCLUDE_DIRS "."
    REQUIRES esp_tinyusb driver freertos keyboard config_module
)