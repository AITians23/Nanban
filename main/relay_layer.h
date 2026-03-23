#ifndef _RELAY_LAYER_H
#define _RELAY_LAYER_H

/**
 * @brief Configure fogger, motor, and pump GPIOs as outputs; all OFF.
 */
void relay_layer_init(void);

/**
 * @brief Apply MQTT-style control: @p control is "fogger"|"motor"|"pump", @p state is "ON"|"OFF".
 */
void relay_layer_apply(const char *control, const char *state);

const char *relay_layer_fogger_state(void);
const char *relay_layer_motor_state(void);
const char *relay_layer_pump_state(void);

#endif /* _RELAY_LAYER_H */
