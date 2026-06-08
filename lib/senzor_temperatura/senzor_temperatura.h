#pragma once

#include <stdint.h>
#include "esp_err.h"

#define TEMP_SENSOR_GPIO 27

void temp_sensor_init(void);
void temp_sensor_task(void *pvParameters);
esp_err_t temp_sensor_read(float *temperature, float *humidity);

float temp_sensor_get_temperature(void); 
float temp_sensor_get_humidity(void);    
