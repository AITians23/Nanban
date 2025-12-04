idf_component_register(
    SRCS "main.c" "water_sensor.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_log
)