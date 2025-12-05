#ifndef WATER_SENSOR_H
#define WATER_SENSOR_H

# include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "driver/gpio.h"

void water_sensor_init(gpio_num_t gpio);
int water_sensor_read(gpio_num_t gpio);
void wifi_init(void);
void init_nvs();
void mqtt_start(void);
void publish_water_status(int water_state);

#endif