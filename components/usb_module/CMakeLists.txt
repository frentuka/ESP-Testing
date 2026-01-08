idf_component_register(
    SRCS "usbmod.c"
    INCLUDE_DIRS "."
    REQUIRES esp_tinyusb driver freertos
)