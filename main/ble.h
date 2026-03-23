#ifndef _BLE_H
#define _BLE_H

#include "ble_layer.h"

/**
 * @brief Handler for @c ble_layer_start: NVS-oriented debug commands (SET_WIFI, SET_BROKER, RESET_NVS).
 */
void ble_debug_layer_on_command(const char *command, const char *arg1, const char *arg2);

#endif /* _BLE_H */
