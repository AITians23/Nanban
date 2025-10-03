#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define SENSOR_GPIO GPIO_NUM_2   // water sensor connected to GPIO2
#define TAG "WATER_SENSOR"

static int last_state = -1;   // to store the previous state (-1 = undefined)

void app_main(void)
{
    // Configure GPIO as input
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SENSOR_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Water sensor initialized on GPIO %d", SENSOR_GPIO);

    while (1) {
        int current_state = gpio_get_level(SENSOR_GPIO);

        // Check if the state has changed
        if (current_state != last_state) {
            if (current_state == 1) {
                ESP_LOGI(TAG, " WATER DETECTED!");
            } else {
                ESP_LOGI(TAG, "NO WATER!");
            }
            last_state = current_state; // update stored state
        }

        vTaskDelay(pdMS_TO_TICKS(200));  // check every 200ms
    }
}
