#ifndef _SYSTEM_MAC_H
#define _SYSTEM_MAC_H

#include "esp_err.h"

esp_err_t system_mac_init(void);
const char *system_mac_get_str(void);

#endif /* _SYSTEM_MAC_H */

