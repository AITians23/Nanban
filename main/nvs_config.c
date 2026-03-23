#include "nvs_config.h"

#include <string.h>
#include <stdbool.h>

#include "app_config.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *nvs_namespace = NANBAN_NVS_NAMESPACE;

/**
 * @brief Read a string value from NVS.
 */
static bool nvs_try_read_str(nvs_handle_t handle, const char *key, char *out, size_t out_len)
{
    size_t required = out_len;
    esp_err_t err = nvs_get_str(handle, key, out, &required);

    if (err == ESP_OK)
    {
        return true;
    }

    return false;
}

/**
 * @brief Initialize NVS and ensure storage is ready.
 */
void nvs_config_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        (void)nvs_flash_erase();
        (void)nvs_flash_init();
    }
}

/**
 * @brief Try to load WiFi credentials from NVS.
 */
bool nvs_config_try_get_wifi_credentials(char *ssid_out, size_t ssid_out_len, char *pass_out, size_t pass_out_len)
{
    if (ssid_out == NULL || pass_out == NULL || ssid_out_len == 0 || pass_out_len == 0)
    {
        return false;
    }

    nvs_handle_t handle;
    if (nvs_open(nvs_namespace, NVS_READONLY, &handle) != ESP_OK)
    {
        return false;
    }

    bool got_ssid = nvs_try_read_str(handle, NANBAN_NVS_WIFI_SSID_KEY, ssid_out, ssid_out_len);
    bool got_pass = nvs_try_read_str(handle, NANBAN_NVS_WIFI_PASS_KEY, pass_out, pass_out_len);

    (void)nvs_close(handle);
    return got_ssid && got_pass;
}

/**
 * @brief Try to load MQTT broker URI from NVS.
 */
bool nvs_config_try_get_mqtt_broker(char *broker_out, size_t broker_out_len)
{
    if (broker_out == NULL || broker_out_len == 0)
    {
        return false;
    }

    nvs_handle_t handle;
    if (nvs_open(nvs_namespace, NVS_READONLY, &handle) != ESP_OK)
    {
        return false;
    }

    bool found = nvs_try_read_str(handle, NANBAN_NVS_MQTT_BROKER_KEY, broker_out, broker_out_len);

    (void)nvs_close(handle);
    return found;
}

/**
 * @brief Save WiFi credentials to NVS.
 */
esp_err_t nvs_config_set_wifi_credentials(const char *ssid, const char *pass)
{
    if (ssid == NULL || pass == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(nvs_namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_set_str(handle, NANBAN_NVS_WIFI_SSID_KEY, ssid);
    if (err == ESP_OK)
    {
        err = nvs_set_str(handle, NANBAN_NVS_WIFI_PASS_KEY, pass);
    }
    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }

    (void)nvs_close(handle);
    return err;
}

/**
 * @brief Save MQTT broker URI to NVS.
 */
esp_err_t nvs_config_set_mqtt_broker(const char *broker_uri)
{
    if (broker_uri == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(nvs_namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_set_str(handle, NANBAN_NVS_MQTT_BROKER_KEY, broker_uri);
    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }

    (void)nvs_close(handle);
    return err;
}

/**
 * @brief Reset all stored runtime configuration keys from NVS.
 */
esp_err_t nvs_config_reset_all(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(nvs_namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        return err;
    }

    (void)nvs_erase_key(handle, NANBAN_NVS_WIFI_SSID_KEY);
    (void)nvs_erase_key(handle, NANBAN_NVS_WIFI_PASS_KEY);
    (void)nvs_erase_key(handle, NANBAN_NVS_MQTT_BROKER_KEY);
    err = nvs_commit(handle);
    (void)nvs_close(handle);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "NVS runtime configuration reset");
    }

    return err;
}

