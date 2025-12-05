idf_component_register(
    SRCS "water_sensor.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_wifi nvs_flash mqtt
)
