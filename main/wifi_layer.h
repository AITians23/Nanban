#ifndef _WIFI_LAYER_H
#define _WIFI_LAYER_H

#include <stdbool.h>
#include <stddef.h>

void wifi_layer_start(const char *ssid, const char *pass);
bool wifi_layer_is_connected(void);

#endif /* _WIFI_LAYER_H */