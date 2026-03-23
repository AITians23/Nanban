#ifndef _SENSORS_LAYER_H
#define _SENSORS_LAYER_H

#include <stdbool.h>

typedef struct
{
    float moisture;       // 0..100
    char water_state[5]; // "HIGH" or "LOW"
    int temperature;     // from DHT11
    int humidity;        // from DHT11
} sensor_data_t;

void sensors_layer_init(void);
bool sensors_layer_read(sensor_data_t *out);

/**
 * @brief Start FreeRTOS task: every 3 s, if Wi-Fi and MQTT are up, read sensors and publish JSON telemetry.
 * @note Call after @c mqtt_layer_start (and relay layer init) so MQTT and relay state getters are valid.
 */
void sensors_layer_start_telemetry_task(void);

#endif /* _SENSORS_LAYER_H */

