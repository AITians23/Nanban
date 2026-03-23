#include "sensors_layer.h"

#include <stdlib.h>
#include <string.h>

#include "app_config.h"
#include "json_layer.h"
#include "mqtt_layer.h"
#include "relay_layer.h"
#include "system_mac.h"
#include "wifi_layer.h"

#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static adc_oneshot_unit_handle_t adc_handle;

/**
 * @brief Read DHT11 sensor (temperature + humidity).
 * @param temperature Output temperature in Celsius.
 * @param humidity Output humidity percentage.
 * @return 0 on success, -1 on failure.
 */
static int dht11_read(int *temperature, int *humidity)
{
    uint8_t data[5] = {0};
    int timeout;

    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));

    gpio_set_level(DHT_PIN, 1);
    esp_rom_delay_us(30);

    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    timeout = 0;
    while (gpio_get_level(DHT_PIN) == 1)
    {
        esp_rom_delay_us(1);
        if (++timeout > 10000)
        {
            return -1;
        }
    }

    timeout = 0;
    while (gpio_get_level(DHT_PIN) == 0)
    {
        esp_rom_delay_us(1);
        if (++timeout > 10000)
        {
            return -1;
        }
    }

    timeout = 0;
    while (gpio_get_level(DHT_PIN) == 1)
    {
        esp_rom_delay_us(1);
        if (++timeout > 10000)
        {
            return -1;
        }
    }

    for (int i = 0; i < 40; i++)
    {
        timeout = 0;
        while (gpio_get_level(DHT_PIN) == 0)
        {
            esp_rom_delay_us(1);
            if (++timeout > 10000)
            {
                return -1;
            }
        }

        int width = 0;
        while (gpio_get_level(DHT_PIN) == 1)
        {
            width++;
            esp_rom_delay_us(1);
            if (width > 100)
            {
                break;
            }
        }

        if (width > 40)
        {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
    {
        *humidity = data[0];
        *temperature = data[2];
        return 0;
    }

    return -1;
}

/**
 * @brief Initialize sensors peripherals (ADC for soil, GPIO for water).
 */
void sensors_layer_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };

    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    adc_oneshot_config_channel(adc_handle, SOIL_ADC, &config);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << WATER_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);
}

/**
 * @brief Read all sensors into sensor_data_t.
 * @param out Output structure.
 * @return true on success, false on invalid arguments.
 */
bool sensors_layer_read(sensor_data_t *out)
{
    if (out == NULL)
    {
        return false;
    }

    int adc_raw = 0;
    float moisture = 0.0f;

    adc_oneshot_read(adc_handle, SOIL_ADC, &adc_raw);

    if (adc_raw >= DRY_VALUE)
    {
        moisture = 0;
    }
    else if (adc_raw <= WET_VALUE)
    {
        moisture = 100;
    }
    else
    {
        moisture = ((float)(DRY_VALUE - adc_raw) / (DRY_VALUE - WET_VALUE)) * 100;
    }

    int level = gpio_get_level(WATER_GPIO);
    if (level == 1)
    {
        strncpy(out->water_state, "HIGH", sizeof(out->water_state) - 1);
    }
    else
    {
        strncpy(out->water_state, "LOW", sizeof(out->water_state) - 1);
    }
    out->water_state[sizeof(out->water_state) - 1] = '\0';

    int temperature = 0;
    int humidity = 0;
    if (dht11_read(&temperature, &humidity) != 0)
    {
        ESP_LOGI(TAG, "DHT11 Read Failed");
        temperature = 0;
        humidity = 0;
    }

    out->moisture = moisture;
    out->temperature = temperature;
    out->humidity = humidity;
    return true;
}

static void sensors_telemetry_task(void *pv)
{
    (void)pv;

    while (1)
    {
        if (wifi_layer_is_connected() && mqtt_layer_is_connected())
        {
            sensor_data_t sensor_data = {0};
            if (sensors_layer_read(&sensor_data))
            {
                const char *device_mac = system_mac_get_str();
                char *payload = json_layer_build_feedback_payload(device_mac,
                                                                  &sensor_data,
                                                                  relay_layer_fogger_state(),
                                                                  relay_layer_motor_state(),
                                                                  relay_layer_pump_state());
                if (payload != NULL)
                {
                    ESP_LOGI(TAG, "%s", payload);
                    mqtt_layer_publish_data(payload);
                    free(payload);
                }
            }
        }
        else
        {
            ESP_LOGI(TAG, "Waiting for WiFi/MQTT...");
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void sensors_layer_start_telemetry_task(void)
{
    xTaskCreate(sensors_telemetry_task, "sensor_telemetry_task", 6144, NULL, 5, NULL);
}

