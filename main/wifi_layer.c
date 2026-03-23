#include "wifi_layer.h"

#include <string.h>

#include "app_config.h"
#include "ble_layer.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

static const char *TAG_WIFI = TAG;

static bool wifi_connected = false;

/**
 * @brief WiFi/IP event handler for connection status updates.
 * @param arg Unused.
 */
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG_WIFI, "WiFi disconnected... reconnecting");
        wifi_connected = false;
        esp_wifi_connect();
        ble_layer_ensure_advertising();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG_WIFI, "WiFi Connected");
        wifi_connected = true;
        ble_layer_ensure_advertising();
    }
}

/**
 * @brief Start WiFi in STA mode.
 * @param ssid WiFi SSID.
 * @param pass WiFi password.
 */
void wifi_layer_start(const char *ssid, const char *pass)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = {0},
            .password = {0},
        },
    };

    // Copy while ensuring null termination.
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password) - 1);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    /* WiFi modem PS starves BLE advertising on shared RF (ESP32-S3). Keep PS off so BLE stays scannable. */
    esp_wifi_set_ps(WIFI_PS_NONE);
}

/**
 * @brief Get current WiFi connection state.
 * @return true if connected, else false.
 */
bool wifi_layer_is_connected(void)
{
    return wifi_connected;
}

