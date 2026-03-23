#include "ble_layer.h"

#include <string.h>
#include <ctype.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"

#include "app_config.h"

#define BLE_PROFILE_APP_ID 0x56
#define BLE_DEVICE_NAME_MAX_LEN 31
#define BLE_RX_MAX_LEN 192
#define BLE_NOTIFY_MAX_LEN 256

#define BLE_SVC_UUID 0xABF0
#define BLE_CHAR_UUID_RX 0xABF1
#define BLE_CHAR_UUID_TX 0xABF2

/* Legacy advertising PDU data limit (31 octets). */
#define BLE_ADV_RAW_MAX 31
#define BLE_AD_TYPE_FLAGS 0x01
#define BLE_AD_TYPE_COMPLETE_NAME 0x09

enum
{
    IDX_SVC,
    IDX_CHAR_RX_DECL,
    IDX_CHAR_RX_VAL,
    IDX_CHAR_TX_DECL,
    IDX_CHAR_TX_VAL,
    IDX_CHAR_TX_CCC,
    IDX_NB
};

static const uint16_t uuid_primary_service = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t uuid_char_decl = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t uuid_client_config = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint8_t prop_read_write_nr = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
static const uint8_t prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

static const uint16_t service_uuid = BLE_SVC_UUID;
static const uint16_t char_rx_uuid = BLE_CHAR_UUID_RX;
static const uint16_t char_tx_uuid = BLE_CHAR_UUID_TX;

static const uint8_t char_rx_init[20] = {0x00};
static const uint8_t char_tx_init[20] = {0x00};
static const uint8_t char_tx_ccc[2] = {0x00, 0x00};

static const esp_gatts_attr_db_t gatt_db[IDX_NB] = {
    [IDX_SVC] = {{ESP_GATT_AUTO_RSP},
                 {ESP_UUID_LEN_16, (uint8_t *)&uuid_primary_service, ESP_GATT_PERM_READ,
                  sizeof(service_uuid), sizeof(service_uuid), (uint8_t *)&service_uuid}},

    [IDX_CHAR_RX_DECL] = {{ESP_GATT_AUTO_RSP},
                          {ESP_UUID_LEN_16, (uint8_t *)&uuid_char_decl, ESP_GATT_PERM_READ,
                           sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&prop_read_write_nr}},

    [IDX_CHAR_RX_VAL] = {{ESP_GATT_AUTO_RSP},
                         {ESP_UUID_LEN_16, (uint8_t *)&char_rx_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                          BLE_RX_MAX_LEN, sizeof(char_rx_init), (uint8_t *)char_rx_init}},

    [IDX_CHAR_TX_DECL] = {{ESP_GATT_AUTO_RSP},
                          {ESP_UUID_LEN_16, (uint8_t *)&uuid_char_decl, ESP_GATT_PERM_READ,
                           sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&prop_read_notify}},

    [IDX_CHAR_TX_VAL] = {{ESP_GATT_AUTO_RSP},
                         {ESP_UUID_LEN_16, (uint8_t *)&char_tx_uuid, ESP_GATT_PERM_READ,
                          BLE_NOTIFY_MAX_LEN, sizeof(char_tx_init), (uint8_t *)char_tx_init}},

    [IDX_CHAR_TX_CCC] = {{ESP_GATT_AUTO_RSP},
                         {ESP_UUID_LEN_16, (uint8_t *)&uuid_client_config, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                          sizeof(uint16_t), sizeof(char_tx_ccc), (uint8_t *)char_tx_ccc}},
};

static ble_command_handler_t command_handler = NULL;
static char ble_advertising_name[BLE_DEVICE_NAME_MAX_LEN + 1] = "NANBAN";
static bool ble_stack_initialized = false;
static bool ble_connected = false;
static bool tx_notify_enabled = false;

static esp_gatt_if_t g_gatts_if = ESP_GATT_IF_NONE;
static uint16_t g_conn_id = 0xFFFF;
static uint16_t g_handle_table[IDX_NB] = {0};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .peer_addr = {0},
    .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/**
 * @brief Trim leading/trailing whitespace in-place.
 * @param text Mutable string buffer.
 */
static void trim_in_place(char *text)
{
    size_t len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1]))
    {
        text[len - 1] = '\0';
        len--;
    }

    size_t start = 0;
    while (text[start] != '\0' && isspace((unsigned char)text[start]))
    {
        start++;
    }

    if (start > 0)
    {
        memmove(text, text + start, strlen(text + start) + 1);
    }
}

/**
 * @brief Parse one token from command line.
 * Supports quoted values to allow spaces inside arguments.
 * @param cursor Input/output scan cursor.
 * @return Pointer to token in-place, or NULL if no token.
 */
static char *parse_next_token(char **cursor)
{
    if (cursor == NULL || *cursor == NULL)
    {
        return NULL;
    }

    char *p = *cursor;
    while (*p != '\0' && isspace((unsigned char)*p))
    {
        p++;
    }

    if (*p == '\0')
    {
        *cursor = p;
        return NULL;
    }

    char *token = p;
    if (*p == '"')
    {
        token = p + 1;
        p = token;
        while (*p != '\0' && *p != '"')
        {
            p++;
        }
        if (*p == '"')
        {
            *p = '\0';
            p++;
        }
    }
    else
    {
        while (*p != '\0' && !isspace((unsigned char)*p))
        {
            p++;
        }
        if (*p != '\0')
        {
            *p = '\0';
            p++;
        }
    }

    *cursor = p;
    return token;
}

/**
 * @brief Start BLE advertising if possible.
 */
static void ble_start_advertising(void)
{
    esp_err_t err = esp_ble_gap_start_advertising(&adv_params);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "BLE advertising start failed: %s", esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI(TAG, "BLE advertising started");
    }
}

/**
 * @brief Parse and dispatch one command line.
 * @param raw_command Input command line.
 */
void ble_layer_process_command(const char *raw_command)
{
    if (raw_command == NULL || command_handler == NULL)
    {
        return;
    }

    char local[BLE_RX_MAX_LEN];
    strncpy(local, raw_command, sizeof(local) - 1);
    local[sizeof(local) - 1] = '\0';
    trim_in_place(local);

    if (local[0] == '\0')
    {
        return;
    }

    char *cursor = local;
    char *cmd = parse_next_token(&cursor);
    char *arg1 = parse_next_token(&cursor);
    char *arg2 = parse_next_token(&cursor);

    if (cmd == NULL)
    {
        return;
    }

    command_handler(cmd, arg1, arg2);
}

/**
 * @brief GAP callback handler.
 */
static void ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ble_start_advertising();
        break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "BLE adv start failed, status=0x%x", param->adv_start_cmpl.status);
        }
        break;

    default:
        break;
    }
}

/**
 * @brief Main GATTS profile event handler.
 */
static void ble_gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                            esp_gatt_if_t gatts_if,
                                            esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
    {
        g_gatts_if = gatts_if;
        esp_ble_gap_set_device_name(ble_advertising_name);

        /* Primary: flags + name in one raw PDU (dryfire-fw-app style raw adv; flags added for spec-friendly discoverability). */
        uint8_t adv_raw[BLE_ADV_RAW_MAX];
        size_t name_len = strlen(ble_advertising_name);
        if (name_len > 26)
        {
            name_len = 26;
        }
        size_t ri = 0;
        adv_raw[ri++] = 2;
        adv_raw[ri++] = ESP_BLE_AD_TYPE_FLAG;
        adv_raw[ri++] = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT;
        adv_raw[ri++] = (uint8_t)(name_len + 1);
        adv_raw[ri++] = BLE_AD_TYPE_COMPLETE_NAME;
        memcpy(&adv_raw[ri], ble_advertising_name, name_len);
        ri += name_len;
        esp_err_t adv_err = esp_ble_gap_config_adv_data_raw(adv_raw, ri);
        if (adv_err != ESP_OK)
        {
            ESP_LOGE(TAG, "config_adv_data_raw failed: %s", esp_err_to_name(adv_err));
        }

        esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, IDX_NB, 0);
        break;
    }

    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        if (param->add_attr_tab.status == ESP_GATT_OK && param->add_attr_tab.num_handle == IDX_NB)
        {
            memcpy(g_handle_table, param->add_attr_tab.handles, sizeof(g_handle_table));
            esp_ble_gatts_start_service(g_handle_table[IDX_SVC]);
            ESP_LOGI(TAG, "BLE GATT service started");
        }
        else
        {
            ESP_LOGE(TAG, "BLE attr table creation failed, status=0x%x", param->add_attr_tab.status);
        }
        break;

    case ESP_GATTS_CONNECT_EVT:
        ble_connected = true;
        g_conn_id = param->connect.conn_id;
        g_gatts_if = gatts_if;
        ESP_LOGI(TAG, "BLE client connected");
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        ble_connected = false;
        tx_notify_enabled = false;
        g_conn_id = 0xFFFF;
        ESP_LOGI(TAG, "BLE client disconnected");
        ble_start_advertising();
        break;

    case ESP_GATTS_WRITE_EVT:
        if (param->write.handle == g_handle_table[IDX_CHAR_RX_VAL] && !param->write.is_prep)
        {
            uint16_t copy_len = param->write.len;
            if (copy_len >= BLE_RX_MAX_LEN)
            {
                copy_len = BLE_RX_MAX_LEN - 1;
            }

            char command_line[BLE_RX_MAX_LEN];
            memcpy(command_line, param->write.value, copy_len);
            command_line[copy_len] = '\0';
            ble_layer_process_command(command_line);
        }
        else if (param->write.handle == g_handle_table[IDX_CHAR_TX_CCC] && param->write.len == 2)
        {
            uint16_t ccc = (uint16_t)param->write.value[1] << 8 | param->write.value[0];
            tx_notify_enabled = (ccc == 0x0001);
            ESP_LOGI(TAG, "BLE notify %s", tx_notify_enabled ? "enabled" : "disabled");
        }
        break;

    default:
        break;
    }
}

/**
 * @brief Top-level GATTS callback router.
 */
static void ble_gatts_event_handler(esp_gatts_cb_event_t event,
                                    esp_gatt_if_t gatts_if,
                                    esp_ble_gatts_cb_param_t *param)
{
    ble_gatts_profile_event_handler(event, gatts_if, param);
}

/**
 * @brief Initialize BLE controller + stack + callbacks.
 * @return ESP_OK on success.
 */
static esp_err_t ble_stack_init(void)
{
    esp_err_t err;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    err = esp_bt_controller_init(&bt_cfg);
    if (err != ESP_OK)
    {
        return err;
    }

    err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (err != ESP_OK)
    {
        return err;
    }

    err = esp_bluedroid_init();
    if (err != ESP_OK)
    {
        return err;
    }

    err = esp_bluedroid_enable();
    if (err != ESP_OK)
    {
        return err;
    }

    err = esp_ble_gatts_register_callback(ble_gatts_event_handler);
    if (err != ESP_OK)
    {
        return err;
    }

    err = esp_ble_gap_register_callback(ble_gap_event_handler);
    if (err != ESP_OK)
    {
        return err;
    }

    err = esp_ble_gatts_app_register(BLE_PROFILE_APP_ID);
    if (err != ESP_OK)
    {
        return err;
    }

    err = esp_ble_gatt_set_local_mtu(500);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "BLE set MTU failed: %s", esp_err_to_name(err));
    }

    return ESP_OK;
}

/**
 * @brief Start BLE debug command layer with full GAP+GATTS.
 * @param advertising_name BLE advertising/device name.
 * @param on_command Callback for parsed commands.
 */
void ble_layer_start(const char *advertising_name, ble_command_handler_t on_command)
{
    command_handler = on_command;

    if (advertising_name != NULL)
    {
        strncpy(ble_advertising_name, advertising_name, sizeof(ble_advertising_name) - 1);
        ble_advertising_name[sizeof(ble_advertising_name) - 1] = '\0';
    }

    if (ble_stack_initialized)
    {
        ESP_LOGI(TAG, "BLE already initialized");
        return;
    }

    esp_err_t err = ble_stack_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "BLE init failed: %s", esp_err_to_name(err));
        return;
    }

    ble_stack_initialized = true;
    ESP_LOGI(TAG, "BLE layer started");
    ESP_LOGI(TAG, "BLE advertising name: %s", ble_advertising_name);
    ESP_LOGI(TAG, "BLE command format:");
    ESP_LOGI(TAG, "  SET_WIFI <ssid> <password>");
    ESP_LOGI(TAG, "  SET_BROKER <broker_uri>");
    ESP_LOGI(TAG, "  RESET_NVS");
}

/**
 * @brief Check whether BLE has an active connection.
 * @return true if connected.
 */
void ble_layer_ensure_advertising(void)
{
    if (!ble_stack_initialized || ble_connected)
    {
        return;
    }
    ble_start_advertising();
}

bool ble_layer_is_connected(void)
{
    return ble_connected;
}

/**
 * @brief Send BLE notification to connected client.
 * @param data Notification payload.
 * @param length Payload length.
 */
void ble_layer_send_notification(const uint8_t *data, uint16_t length)
{
    if (!ble_connected || !tx_notify_enabled || g_conn_id == 0xFFFF || g_gatts_if == ESP_GATT_IF_NONE)
    {
        return;
    }

    if (data == NULL || length == 0)
    {
        return;
    }

    uint16_t send_len = length;
    if (send_len > BLE_NOTIFY_MAX_LEN)
    {
        send_len = BLE_NOTIFY_MAX_LEN;
    }

    esp_ble_gatts_send_indicate(g_gatts_if, g_conn_id, g_handle_table[IDX_CHAR_TX_VAL], send_len, (uint8_t *)data, false);
}

