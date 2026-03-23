# Application entry point is main_app.c (app_main). Do not add a second main.c — ESP-IDF allows any .c name in SRCS.
idf_component_register(SRCS "system_mac.c" "ble_layer.c" "ble_debug_layer.c" "main_app.c" "mqtt_layer.c" "json_layer.c" "relay_layer.c" "sensors_layer.c" "wifi_layer.c" "nvs_config.c"
                    INCLUDE_DIRS "."
                    REQUIRES bt driver esp_adc nvs_flash mqtt json esp_wifi esp_netif esp_event)
