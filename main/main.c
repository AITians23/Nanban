#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "soil_moisture";
static adc_oneshot_unit_handle_t adc_handle;

// Typical calibration values for HW-390 capacitive soil moisture sensor
// Calibrate these by measuring ADC in air (dry) and in water (wet)
#define DRY_VALUE  3100  // ADC value in dry air/soil
#define WET_VALUE  1200  // ADC value in wet soil/water
#define ADC_PIN    ADC1_CHANNEL_6  // GPIO7 on ESP32-S3, ADC1_CH1

void app_main(void)
{
    // Configure ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,  // For 0-3.3V range
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_PIN, &config));

    int adc_raw = 0;
    float moisture_percent = 0.0f;

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_PIN, &adc_raw));

        // Map to moisture percentage: higher ADC = drier
        if (adc_raw >= DRY_VALUE) {
            moisture_percent = 0.0f;
        } else if (adc_raw <= WET_VALUE) {
            moisture_percent = 100.0f;
        } else {
            moisture_percent = ((float)(DRY_VALUE - adc_raw) / (DRY_VALUE - WET_VALUE)) * 100.0f;
        }

        ESP_LOGI(TAG, "ADC Raw: %d, Moisture: %.1f%%", adc_raw, moisture_percent);

        vTaskDelay(pdMS_TO_TICKS(1000));  // Read every 1 second
    }
}