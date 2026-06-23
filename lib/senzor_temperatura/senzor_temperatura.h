#pragma once

#include <stdint.h>
#include "esp_err.h"

#define TEMP_SENSOR_GPIO 27

void temp_sensor_init(void);
void temp_sensor_task(void *pvParameters);

esp_err_t temp_sensor_read(uint16_t *temp_tenths, uint8_t *temp_negative,
                           uint16_t *hum_tenths);

uint16_t temp_sensor_get_temp_tenths(void);
uint8_t  temp_sensor_get_temp_negative(void);
uint16_t temp_sensor_get_hum_tenths(void);
