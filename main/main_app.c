#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "app_config.h"
#include "ble.h"
#include "mqtt_layer.h"
#include "nvs_config.h"
#include "relay_layer.h"
#include "sensors_layer.h"
#include "system_mac.h"
#include "wifi_layer.h"

/**
 * @brief ESP-IDF main entry point.
 *
 * Wires NVS, identity, sensor/relay/BLE/Wi-Fi/MQTT layers and starts telemetry task.
 */
void app_main(void)
{
    nvs_config_init();

    (void)system_mac_init();
    const char *device_mac = system_mac_get_str();

    char ble_adv_name[32];
    snprintf(ble_adv_name, sizeof(ble_adv_name), "NANBAN-%s", device_mac);

    char ssid[NVS_MAX_SSID_LEN] = {0};
    char pass[NVS_MAX_PASS_LEN] = {0};
    char broker[NVS_MAX_BROKER_LEN] = {0};

    strncpy(ssid, WIFI_SSID_DEFAULT, sizeof(ssid) - 1);
    strncpy(pass, WIFI_PASS_DEFAULT, sizeof(pass) - 1);
    strncpy(broker, MQTT_BROKER_DEFAULT, sizeof(broker) - 1);

    bool has_wifi_override = nvs_config_try_get_wifi_credentials(ssid, sizeof(ssid), pass, sizeof(pass));
    bool has_broker_override = nvs_config_try_get_mqtt_broker(broker, sizeof(broker));

    if (has_wifi_override)
    {
        ESP_LOGI(TAG, "Using WiFi credentials from NVS");
    }
    else
    {
        ESP_LOGI(TAG, "Using default WiFi credentials");
    }

    if (has_broker_override)
    {
        ESP_LOGI(TAG, "Using MQTT broker from NVS");
    }
    else
    {
        ESP_LOGI(TAG, "Using default MQTT broker");
    }

    sensors_layer_init();
    relay_layer_init();
    ble_layer_start(ble_adv_name, ble_debug_layer_on_command);

    /* Let GATT register and first adv complete before Wi-Fi uses the same RF (dryfire-fw-app starts BLE then Wi-Fi later). */
    vTaskDelay(pdMS_TO_TICKS(300));

    wifi_layer_start(ssid, pass);

    /* MQTT retries until the interface has an address; do not block here or BLE stays marginal until Wi-Fi connects. */
    mqtt_layer_start(broker,
                     MQTT_CONTROL_TOPIC,
                     MQTT_DATA_TOPIC,
                     device_mac,
                     relay_layer_apply,
                     (MQTT_AUTO_RECONNECT_DEFAULT != 0),
                     MQTT_RECONNECT_TIMEOUT_MS_DEFAULT);

    sensors_layer_start_telemetry_task();
}
