idf_component_register(
    SRCS
        "enmod.c"
        "enmod_inout.c"
        "enmod_basics.c"
        "enmod_machine.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver freertos
    REQUIRES esp_timer wifi_module mbedtls
)