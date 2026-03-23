#include "system_mac.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_mac.h"

#include "app_config.h"

static bool mac_initialized = false;
static char device_mac_str[18] = "00:00:00:00:00:00";

/**
 * @brief Initialize system identity (formatted device MAC string).
 * @return ESP_OK on success, or an ESP-IDF error code.
 */
esp_err_t system_mac_init(void)
{
    if (mac_initialized)
    {
        return ESP_OK;
    }

    uint8_t device_mac[6] = {0};

    // Uses BT MAC just like your snippet.
    esp_err_t ret = esp_read_mac(device_mac, ESP_MAC_BT);
    if (ret != ESP_OK)
    {
        strncpy(device_mac_str, "00:00:00:00:00:00", sizeof(device_mac_str));
        mac_initialized = true;

        ESP_LOGE(TAG, "Failed to read MAC address: %s", esp_err_to_name(ret));
        return ret;
    }

    snprintf(device_mac_str,
             sizeof(device_mac_str),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             device_mac[0],
             device_mac[1],
             device_mac[2],
             device_mac[3],
             device_mac[4],
             device_mac[5]);

    mac_initialized = true;

    ESP_LOGI(TAG, "Device MAC address: %s", device_mac_str);
    return ESP_OK;
}

/**
 * @brief Get formatted device MAC string.
 * @return Pointer to internal null-terminated MAC string.
 */
const char *system_mac_get_str(void)
{
    return device_mac_str;
}

