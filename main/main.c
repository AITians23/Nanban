#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "water_sensor.h"

#define WATER_SENSOR_GPIO 4

void app_main(void)
{
    ESP_LOGI("MAIN", "Starting system...");

    // Start WiFi + MQTT from water_sensor.c
    wifi_init();
    mqtt_start();

    // while (1)
    // {
    //     int water_level = water_sensor_read(WATER_SENSOR_GPIO);

    //     ESP_LOGI("MAIN", "Water Level: %d", water_level);

    //     // Publish water level to MQTT
    //     publish_water_status(water_level);

    //     vTaskDelay(pdMS_TO_TICKS(2000));
    // }
}
