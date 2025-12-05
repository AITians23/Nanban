#ifndef WATER_SENSOR_H
#define WATER_SENSOR_H

#include "driver/gpio.h"

void water_sensor_init(gpio_num_t gpio);
int water_sensor_read(gpio_num_t gpio);

#endif