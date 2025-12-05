#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "water_sensor.h"

#define WATER_SENSOR_GPIO 4

void app_main(void)
{
    // water_sensor_init(WATER_SENSOR_GPIO);

    while (1)
    {
        int water_level = water_sensor_read(WATER_SENSOR_GPIO);
        ESP_LOGI("MAIN", "Water Level: %d", water_level);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}