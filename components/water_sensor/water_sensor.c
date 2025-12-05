#include "water_sensor.h"    // Your custom component

// Macros
#define WIFI_SSID      "Radinnolabs"
#define WIFI_PASS      "Rlabs@2023"

#define MQTT_BROKER_URI "mqtts://esp32user:tnau1234@302e0df2bf084106ae7ee162f3ab3471.s1.eu.hivemq.cloud:8883"

#define WATER_GPIO     15

static const char *TAG = "FINAL_PROJECT";

// Global MQTT client
static esp_mqtt_client_handle_t client = NULL;
static QueueHandle_t water_event_queue;

// =====================================================
// MQTT Event Handler
// =====================================================
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT Disconnected");
            break;

        default:
            break;
    }
}

// =====================================================
// Wi-Fi Event Handler
// =====================================================
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "WiFi Started → Connecting...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG, "WiFi Disconnected → Reconnecting...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "WiFi Connected! Got IP Address.");
    }
}

void init_nvs()
{
    // 1. Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

}

// =====================================================
// Wi-Fi Initialization (Correct ESP-IDF v5.x Format)
// =====================================================
void wifi_init(void)
{
    init_nvs();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register handlers
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));

    // Proper WiFi config for ESP-IDF v5.x
    wifi_config_t wifi_config = { 0 };

    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, WIFI_PASS);

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Start Wi-Fi Driver
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi Initialized");
}

// =====================================================
// MQTT Start (Secure HiveMQ Cloud)
// =====================================================
void mqtt_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .broker.verification.skip_cert_common_name_check = true,
        .session.keepalive = 30,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client,
                                   ESP_EVENT_ANY_ID,
                                   mqtt_event_handler,
                                   NULL);

    esp_mqtt_client_start(client);
}

// =====================================================
// Publish Water Status
// =====================================================
void publish_water_status(int water_state)
{
    if (!client)
    {
        ESP_LOGW(TAG, "MQTT not ready!");
        return;
    }

    char msg[64];
    snprintf(msg, sizeof(msg),
             "{\"water_level\":%d}", water_state);

    esp_mqtt_client_publish(client,
                            "esp32/water_level",
                            msg, 0, 1, 0);

    ESP_LOGI(TAG, "MQTT Published → %s", msg);
}

// =====================================================
// Water Sensor Queue Processing Task
// =====================================================
static void water_event_task(void *pvParameters)
{
    int water_state;

    while (1)
    {
        if (xQueueReceive(water_event_queue,
                          &water_state,
                          portMAX_DELAY))
        {
            ESP_LOGI(TAG,
                     "Water Sensor State: %s",
                     water_state ? "DETECTED" : "NOT DETECTED");

            publish_water_status(water_state);
        }
    }
}