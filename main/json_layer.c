#include "json_layer.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "app_config.h"

#include "cJSON.h"
#include "esp_log.h"

/**
 * @brief Copy a JSON string value into an output buffer.
 */
static void copy_string_or_fail(cJSON *item, char *out, size_t out_len)
{
    if (!cJSON_IsString(item))
        return;
    strncpy(out, item->valuestring, out_len - 1);
    out[out_len - 1] = '\0';
}

/**
 * @brief Build MQTT feedback payload (telemetry JSON).
 * @return Heap-allocated string on success (caller must free()), else NULL.
 */
char *json_layer_build_feedback_payload(const char *mac_address,
                                        const sensor_data_t *sensor_data,
                                        const char *fogger_state,
                                        const char *motor_state,
                                        const char *pump_state)
{
    if (mac_address == NULL || sensor_data == NULL)
        return NULL;

    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
        return NULL;

    // Keep numeric formatting close to your original "soil":%.2f by injecting raw JSON.
    char soil_raw[24];
    float moisture = sensor_data->moisture;
    snprintf(soil_raw, sizeof(soil_raw), "%.2f", moisture);

    cJSON_AddStringToObject(root, "macAddress", mac_address);
    cJSON_AddRawToObject(root, "soil", soil_raw);
    cJSON_AddStringToObject(root, "water", sensor_data->water_state);
    cJSON_AddNumberToObject(root, "temperature", sensor_data->temperature);
    cJSON_AddNumberToObject(root, "humidity", sensor_data->humidity);
    cJSON_AddStringToObject(root, "foggerState", fogger_state);
    cJSON_AddStringToObject(root, "motorState", motor_state);
    cJSON_AddStringToObject(root, "pumpState", pump_state);

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out; // caller must free()
}

/**
 * @brief Parse incoming MQTT control JSON.
 * @return true on success (valid JSON, string @a control and @a action; if @a macAddress is a string it must match @a expected_mac).
 */
bool json_layer_parse_control_payload(const char *json_data,
                                       const char *expected_mac,
                                       char *control_out,
                                       size_t control_out_len,
                                       char *action_out,
                                       size_t action_out_len)
{
    if (json_data == NULL || expected_mac == NULL || control_out == NULL || action_out == NULL)
        return false;

    cJSON *root = cJSON_Parse(json_data);
    if (root == NULL)
    {
        ESP_LOGW(TAG, "Control MQTT: JSON parse failed (payload ignored)");
        return false;
    }

    cJSON *mac = cJSON_GetObjectItem(root, "macAddress");
    cJSON *control = cJSON_GetObjectItem(root, "control");
    cJSON *action = cJSON_GetObjectItem(root, "action");

    if (!cJSON_IsString(control) || !cJSON_IsString(action))
    {
        ESP_LOGW(TAG,
                 "Control MQTT: need string fields control, action (got control=%s action=%s) — payload ignored",
                 control ? (cJSON_IsString(control) ? "ok" : "wrong_type") : "missing",
                 action ? (cJSON_IsString(action) ? "ok" : "wrong_type") : "missing");
        cJSON_Delete(root);
        return false;
    }

    if (cJSON_IsString(mac))
    {
        if (strcmp(mac->valuestring, expected_mac) != 0)
        {
            ESP_LOGW(TAG,
                     "Control MQTT: macAddress \"%s\" != this device \"%s\" — payload ignored",
                     mac->valuestring,
                     expected_mac);
            cJSON_Delete(root);
            return false;
        }
    }
    else if (mac != NULL)
    {
        ESP_LOGW(TAG, "Control MQTT: macAddress not a string — skipped MAC check, applying command");
    }

    copy_string_or_fail(control, control_out, control_out_len);
    copy_string_or_fail(action, action_out, action_out_len);

    cJSON_Delete(root);
    return true;
}

