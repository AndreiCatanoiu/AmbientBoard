#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define WIFI_SSID     "TotalRom_2.4GHz"
#define WIFI_PASSWORD "totalrom"


esp_err_t wifi_init(void);
void wifi_task(void *pvParameters);
bool wifi_is_connected(void);
