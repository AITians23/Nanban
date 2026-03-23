#ifndef _BLE_LAYER_H
#define _BLE_LAYER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef void (*ble_command_handler_t)(const char *command, const char *arg1, const char *arg2);

void ble_layer_start(const char *advertising_name, ble_command_handler_t on_command);
void ble_layer_process_command(const char *raw_command);
bool ble_layer_is_connected(void);
void ble_layer_send_notification(const uint8_t *data, uint16_t length);
void ble_layer_ensure_advertising(void);

#endif /* _BLE_LAYER_H */

