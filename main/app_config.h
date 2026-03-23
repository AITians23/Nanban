#ifndef _APP_CONFIG_H
#define _APP_CONFIG_H

#include <stddef.h>

// Provides GPIO_NUM_x and ADC_CHANNEL_x macros.
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

// Default credentials if NVS doesn't have stored values yet.
#define WIFI_SSID_DEFAULT "OPPO A38"
#define WIFI_PASS_DEFAULT "12345678"

// Default MQTT broker URI (must include scheme and port).
#define MQTT_BROKER_DEFAULT "mqtt://broker.emqx.io:1883"

// MQTT client: automatic reconnect and delay between attempts (ms). Non-positive timeout uses default below.
#define MQTT_AUTO_RECONNECT_DEFAULT         1
#define MQTT_RECONNECT_TIMEOUT_MS_DEFAULT   10000

// MQTT topics.
#define MQTT_DATA_TOPIC "nanban/data"
#define MQTT_CONTROL_TOPIC "nanban/control"

// GPIO pins (ESP32/ESP-IDF).
#define FOGGER_GPIO GPIO_NUM_5
#define MOTOR_GPIO GPIO_NUM_18
#define PUMP_GPIO GPIO_NUM_19

// Soil sensor via ADC.
#define DRY_VALUE 3100
#define WET_VALUE 1200
#define SOIL_ADC ADC_CHANNEL_6

// Water sensor GPIO.
#define WATER_GPIO GPIO_NUM_2

// DHT11 GPIO.
#define DHT_PIN 4

// NVS config.
#define NANBAN_NVS_NAMESPACE "nanban_cfg"
#define NANBAN_NVS_WIFI_SSID_KEY "wifi_ssid"
#define NANBAN_NVS_WIFI_PASS_KEY "wifi_pass"
#define NANBAN_NVS_MQTT_BROKER_KEY "mqtt_broker"

// Buffer sizes for NVS strings.
#define NVS_MAX_SSID_LEN 33
#define NVS_MAX_PASS_LEN 65
#define NVS_MAX_BROKER_LEN 128

// Logging tag.
#define TAG "NANBAN"

// Keep this in sync with relay state buffer sizes in main.c.
#define RELAY_STATE_STR_LEN 4

#endif /* _APP_CONFIG_H */