#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rom/ets_sys.h"
 // For esp_rom_delay_us()

#define DHT11_PIN GPIO_NUM_4   // Change to your connected pin

static const char *TAG = "DHT11";

void dht11_start_signal() {
    gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20)); // Pull low for 20 ms
    gpio_set_level(DHT11_PIN, 1);
    ets_delay_us(30);// 20–40 us high
    gpio_set_direction(DHT11_PIN, GPIO_MODE_INPUT);
}

int dht11_read_bit() {
    while (gpio_get_level(DHT11_PIN) == 0); // wait for pin to go high
    esp_rom_delay_us(40);
    if (gpio_get_level(DHT11_PIN) == 1) {
        while (gpio_get_level(DHT11_PIN) == 1); // wait for low
        return 1;
    }
    return 0;
}

uint8_t dht11_read_byte() {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        value <<= 1;
        value |= dht11_read_bit();
    }
    return value;
}

void dht11_read_data(int *temperature, int *humidity) {
    uint8_t data[5] = {0};

    dht11_start_signal();

    // Wait for DHT11 response
    while (gpio_get_level(DHT11_PIN) == 1);
    while (gpio_get_level(DHT11_PIN) == 0);
    while (gpio_get_level(DHT11_PIN) == 1);

    // Read 5 bytes
    for (int i = 0; i < 5; i++) {
        data[i] = dht11_read_byte();
    }

    if ((data[0] + data[1] + data[2] + data[3]) == data[4]) {
        *humidity = data[0];
        *temperature = data[2];
    } else {
        ESP_LOGE(TAG, "Checksum error");
    }
}

void app_main(void) {
    int temperature = 0, humidity = 0;

    while (1) {
        dht11_read_data(&temperature, &humidity);
        ESP_LOGI(TAG, "Humidity: %d %%  |  Temperature: %d °C", humidity, temperature);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
