#ifndef _MQTT_LAYER_H
#define _MQTT_LAYER_H

#include <stdbool.h>

typedef void (*control_command_handler_t)(const char *control, const char *action);

/**
 * @param auto_reconnect If true, ESP-MQTT reconnects after disconnect (see network.disable_auto_reconnect).
 * @param reconnect_timeout_ms Interval between reconnect attempts; if <= 0, uses MQTT_RECONNECT_TIMEOUT_MS_DEFAULT.
 */
void mqtt_layer_start(const char *broker_uri,
                       const char *control_topic,
                       const char *data_topic,
                       const char *expected_mac,
                       control_command_handler_t on_control,
                       bool auto_reconnect,
                       int reconnect_timeout_ms);
bool mqtt_layer_is_connected(void);
bool mqtt_layer_publish_data(const char *payload);

#endif /* _MQTT_LAYER_H */

