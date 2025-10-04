#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define SENSOR_GPIO GPIO_NUM_2
#define TAG "WATER_SENSOR"

// ISR — runs on state change
static void IRAM_ATTR water_isr(void* arg) {
    int st = gpio_get_level(SENSOR_GPIO);
    if (st == 1) {
        ESP_EARLY_LOGI(TAG, "WATER DETECTED!");
    } else {
        ESP_EARLY_LOGI(TAG, "NO WATER!");
    }
}

void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SENSOR_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE   // trigger on both rising & falling
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(SENSOR_GPIO, water_isr, NULL);

    ESP_LOGI(TAG, "Interrupt based water sensor on GPIO %d", SENSOR_GPIO);

    // no while loop — app_main ends, ISR will keep working
}
