idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS "."
                       REQUIRES button_module rgb_module wifi_module espnow_module usb_module)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)