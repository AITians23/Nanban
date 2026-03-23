#include "ble.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "nvs_config.h"
#include "esp_err.h"
#include "esp_log.h"

static void send_ble_response(const char *format, ...)
{
    if (format == NULL)
    {
        return;
    }

    char message[220];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    ble_layer_send_notification((const uint8_t *)message, (uint16_t)strlen(message));
}

void ble_debug_layer_on_command(const char *command, const char *arg1, const char *arg2)
{
    if (command == NULL)
    {
        return;
    }

    if (strcmp(command, "SET_WIFI") == 0)
    {
        if (arg1 == NULL || arg2 == NULL)
        {
            ESP_LOGI(TAG, "Usage: SET_WIFI \"<ssid>\" \"<password>\"");
            send_ble_response("ERR SET_WIFI usage: SET_WIFI \"<ssid>\" \"<password>\"");
            return;
        }

        esp_err_t err = nvs_config_set_wifi_credentials(arg1, arg2);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "WiFi credentials saved to NVS");
            send_ble_response("OK SET_WIFI ssid=\"%s\" password_len=%u", arg1, (unsigned)strlen(arg2));
        }
        else
        {
            ESP_LOGE(TAG, "SET_WIFI failed: %s", esp_err_to_name(err));
            send_ble_response("ERR SET_WIFI %s", esp_err_to_name(err));
        }
        return;
    }

    if (strcmp(command, "SET_BROKER") == 0)
    {
        if (arg1 == NULL)
        {
            ESP_LOGI(TAG, "Usage: SET_BROKER \"<broker_uri>\"");
            send_ble_response("ERR SET_BROKER usage: SET_BROKER \"<broker_uri>\"");
            return;
        }

        esp_err_t err = nvs_config_set_mqtt_broker(arg1);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "MQTT broker saved to NVS");
            send_ble_response("OK SET_BROKER broker=\"%s\"", arg1);
        }
        else
        {
            ESP_LOGE(TAG, "SET_BROKER failed: %s", esp_err_to_name(err));
            send_ble_response("ERR SET_BROKER %s", esp_err_to_name(err));
        }
        return;
    }

    if (strcmp(command, "RESET_NVS") == 0)
    {
        esp_err_t err = nvs_config_reset_all();
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "NVS settings reset");
            send_ble_response("OK RESET_NVS");
        }
        else
        {
            ESP_LOGE(TAG, "RESET_NVS failed: %s", esp_err_to_name(err));
            send_ble_response("ERR RESET_NVS %s", esp_err_to_name(err));
        }
        return;
    }

    ESP_LOGI(TAG, "Unknown command: %s", command);
    send_ble_response("ERR UNKNOWN_CMD \"%s\"", command);
}
