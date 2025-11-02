idf_component_register(
    SRCS "new_espnow.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver freertos
    REQUIRES esp_timer
)