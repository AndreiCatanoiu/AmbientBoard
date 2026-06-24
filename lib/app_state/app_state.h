#pragma once

#include <stdint.h>

typedef struct {
    uint16_t temp_tenths; 
    uint8_t  temp_negative; 
    uint16_t hum_tenths;   
    uint8_t  valid;        
} app_sensor_t;

typedef struct {
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  day;
    uint8_t  month;
    uint16_t year;
    uint8_t  synced;       
} app_time_t;

typedef struct {
    uint8_t connected;    
    uint8_t strength;      
    char    ip[16];        
    char    ssid[33];      
} app_wifi_t;

void app_state_init(void);

void app_state_set_sensor(const app_sensor_t *s);
void app_state_get_sensor(app_sensor_t *out);

void app_state_set_time(const app_time_t *t);
void app_state_get_time(app_time_t *out);

void app_state_set_wifi(const app_wifi_t *w);
void app_state_get_wifi(app_wifi_t *out);
