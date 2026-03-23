#ifndef _NVS_CONFIG_H
#define _NVS_CONFIG_H

#include <stddef.h>
#include <stdbool.h>

#include "esp_err.h"

void nvs_config_init(void);
bool nvs_config_try_get_wifi_credentials(char *ssid_out, size_t ssid_out_len, char *pass_out, size_t pass_out_len);
bool nvs_config_try_get_mqtt_broker(char *broker_out, size_t broker_out_len);
esp_err_t nvs_config_set_wifi_credentials(const char *ssid, const char *pass);
esp_err_t nvs_config_set_mqtt_broker(const char *broker_uri);
esp_err_t nvs_config_reset_all(void);

#endif /* _NVS_CONFIG_H */