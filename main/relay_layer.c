#include "relay_layer.h"

#include <string.h>

#include "app_config.h"

#include "driver/gpio.h"
#include "esp_log.h"

static char fogger_state[RELAY_STATE_STR_LEN] = "OFF";
static char motor_state[RELAY_STATE_STR_LEN] = "OFF";
static char pump_state[RELAY_STATE_STR_LEN] = "OFF";

static void set_relay_level(gpio_num_t pin, const char *state)
{
    gpio_set_level(pin, strcmp(state, "ON") == 0 ? 1 : 0);
}

void relay_layer_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FOGGER_GPIO) | (1ULL << MOTOR_GPIO) | (1ULL << PUMP_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_conf);

    set_relay_level(FOGGER_GPIO, "OFF");
    set_relay_level(MOTOR_GPIO, "OFF");
    set_relay_level(PUMP_GPIO, "OFF");
}

void relay_layer_apply(const char *control, const char *state)
{
    if (control == NULL || state == NULL)
    {
        return;
    }

    if (strcmp(control, "fogger") == 0)
    {
        strncpy(fogger_state, state, sizeof(fogger_state) - 1);
        fogger_state[sizeof(fogger_state) - 1] = '\0';
        set_relay_level(FOGGER_GPIO, fogger_state);
        ESP_LOGI(TAG, "Fogger -> %s", fogger_state);
    }
    else if (strcmp(control, "motor") == 0)
    {
        strncpy(motor_state, state, sizeof(motor_state) - 1);
        motor_state[sizeof(motor_state) - 1] = '\0';
        set_relay_level(MOTOR_GPIO, motor_state);
        ESP_LOGI(TAG, "Motor -> %s", motor_state);
    }
    else if (strcmp(control, "pump") == 0)
    {
        strncpy(pump_state, state, sizeof(pump_state) - 1);
        pump_state[sizeof(pump_state) - 1] = '\0';
        set_relay_level(PUMP_GPIO, pump_state);
        ESP_LOGI(TAG, "Pump -> %s", pump_state);
    }
}

const char *relay_layer_fogger_state(void)
{
    return fogger_state;
}

const char *relay_layer_motor_state(void)
{
    return motor_state;
}

const char *relay_layer_pump_state(void)
{
    return pump_state;
}
