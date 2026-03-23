#ifndef _JSON_LAYER_H
#define _JSON_LAYER_H

#include <stdbool.h>
#include <stddef.h>

#include "sensors_layer.h"

char *json_layer_build_feedback_payload(const char *mac_address,
                                        const sensor_data_t *sensor_data,
                                        const char *fogger_state,
                                        const char *motor_state,
                                        const char *pump_state);
bool json_layer_parse_control_payload(const char *json_data,
                                       const char *expected_mac,
                                       char *control_out,
                                       size_t control_out_len,
                                       char *action_out,
                                       size_t action_out_len);

#endif /* _JSON_LAYER_H */

