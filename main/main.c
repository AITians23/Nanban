#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define SENSOR_GPIO GPIO_NUM_2  // GPIO2 as input for sensor OUT (after divider)
#define TAG "LIQUID_SENSOR"    // Log tag

void app_main(void)
{
    // Configure GPIO as input with internal pull-up (optional, but good practice)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SENSOR_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Liquid level sensor initialized on GPIO%d", SENSOR_GPIO);

    while (1) {
        int level = gpio_get_level(SENSOR_GPIO);
        
        if (level == 1) {
            ESP_LOGI(TAG, "Liquid DETECTED (HIGH)");
        } else {
            ESP_LOGI(TAG, "No liquid (LOW)");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));  // Delay 500ms
    }
}