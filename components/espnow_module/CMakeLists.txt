idf_component_register(
    SRCS "espnow.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver freertos
    REQUIRES esp_timer wifi_module
)