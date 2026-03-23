#include "mqtt_layer.h"

#include <string.h>
#include <stdlib.h>

#include "app_config.h"

#include "mqtt_client.h"
#include "cJSON.h"
#include "esp_log.h"

#include "json_layer.h"

static esp_mqtt_client_handle_t client = NULL;
static bool mqtt_connected = false;

static char control_topic_buf[64];
static char data_topic_buf[64];
static char expected_mac_buf[32];

static control_command_handler_t on_control_cb = NULL;

/**
 * @brief MQTT event handler (connect/disconnect/data).
 */
static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    (void)handler_args;
    (void)base;

    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connected");
        mqtt_connected = true;
        esp_mqtt_client_subscribe(client, control_topic_buf, 1);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT Disconnected");
        mqtt_connected = false;
        break;

    case MQTT_EVENT_DATA:
    {
        // MQTT payload/topic are not guaranteed to be null-terminated.
        char topic[event->topic_len + 1];
        char data[event->data_len + 1];

        memcpy(topic, event->topic, event->topic_len);
        topic[event->topic_len] = '\0';

        memcpy(data, event->data, event->data_len);
        data[event->data_len] = '\0';

        ESP_LOGI(TAG, "Topic: %s", topic);
        ESP_LOGI(TAG, "Data: %s", data);

        if (strcmp(topic, control_topic_buf) == 0 && on_control_cb != NULL)
        {
            char control[16] = {0};
            char action[16] = {0};

            if (json_layer_parse_control_payload(data, expected_mac_buf, control, sizeof(control), action, sizeof(action)))
            {
                on_control_cb(control, action);
            }
        }
        break;
    }

    default:
        break;
    }
}

/**
 * @brief Start MQTT client and subscribe to control topic.
 * @param broker_uri MQTT broker URI (e.g. mqtt://host:1883).
 * @param control_topic Topic to subscribe for control commands.
 * @param data_topic Topic to publish telemetry to.
 * @param expected_mac Device MAC used to filter incoming control payloads.
 * @param on_control Callback called with parsed control/action strings.
 * @param auto_reconnect Enable ESP-MQTT automatic reconnection.
 * @param reconnect_timeout_ms Delay between reconnect tries (ms); <= 0 uses app default.
 */
void mqtt_layer_start(const char *broker_uri,
                       const char *control_topic,
                       const char *data_topic,
                       const char *expected_mac,
                       control_command_handler_t on_control,
                       bool auto_reconnect,
                       int reconnect_timeout_ms)
{
    if (broker_uri == NULL || control_topic == NULL || data_topic == NULL || expected_mac == NULL)
    {
        return;
    }

    memset(control_topic_buf, 0, sizeof(control_topic_buf));
    memset(data_topic_buf, 0, sizeof(data_topic_buf));
    memset(expected_mac_buf, 0, sizeof(expected_mac_buf));

    strncpy(control_topic_buf, control_topic, sizeof(control_topic_buf) - 1);
    strncpy(data_topic_buf, data_topic, sizeof(data_topic_buf) - 1);
    strncpy(expected_mac_buf, expected_mac, sizeof(expected_mac_buf) - 1);

    on_control_cb = on_control;

    int reconnect_ms = reconnect_timeout_ms;
    if (reconnect_ms <= 0)
    {
        reconnect_ms = MQTT_RECONNECT_TIMEOUT_MS_DEFAULT;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
        .network.disable_auto_reconnect = !auto_reconnect,
        .network.reconnect_timeout_ms = reconnect_ms,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

/**
 * @brief Check current MQTT connectivity state.
 * @return true if connected, else false.
 */
bool mqtt_layer_is_connected(void)
{
    return mqtt_connected;
}

/**
 * @brief Publish telemetry payload to the configured MQTT data topic.
 * @param payload JSON payload (null-terminated).
 * @return true if publish was requested, else false.
 */
bool mqtt_layer_publish_data(const char *payload)
{
    if (client == NULL || payload == NULL || !mqtt_connected)
    {
        return false;
    }

    // QoS=1, retain=0, duplicate=0 to match your original publish settings.
    esp_mqtt_client_publish(client, data_topic_buf, payload, 0, 1, 0);
    return true;
}

