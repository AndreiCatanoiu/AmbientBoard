#pragma once

#include <stdbool.h>
#include <stdint.h>
#define TIME_TZ_ROMANIA "EET-2EEST,M3.5.0/3,M10.5.0/4"

#define TIME_NTP_SERVER "pool.ntp.org"

void time_init(void);

void time_task(void *pvParameters);
bool time_is_synced(void);

uint8_t  time_get_day(void);   
uint8_t  time_get_month(void);  
uint16_t time_get_year(void);   
uint8_t  time_get_hour(void);  
uint8_t  time_get_minute(void); 
uint8_t  time_get_second(void); 

char *time_get_date_string(void);