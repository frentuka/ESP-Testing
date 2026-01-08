idf_component_register(
    SRCS "kb_matrix.c"
    INCLUDE_DIRS "."
    REQUIRES esp_timer driver freertos
)