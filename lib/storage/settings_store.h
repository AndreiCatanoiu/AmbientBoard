#pragma once

#include <stdint.h>
#include <stdbool.h>

#define CAL_TEXT_LEN   32
#define CAL_MAX_EVENTS 32

typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    char     text[CAL_TEXT_LEN];
} calendar_event_t;

void settings_init(void);

uint8_t settings_get_theme(void);
void    settings_set_theme(uint8_t idx);

uint32_t settings_get_led(void);
void     settings_set_led(uint32_t rgb);

bool settings_get_wifi(char *ssid, char *pass);
void settings_set_wifi(const char *ssid, const char *pass);

uint8_t settings_calendar_count(void);
uint8_t settings_calendar_load(calendar_event_t *events, uint8_t max);
bool    settings_calendar_get(uint8_t index, calendar_event_t *out);
bool    settings_calendar_add(const calendar_event_t *ev);
uint8_t settings_calendar_count_for_day(uint16_t year, uint8_t month, uint8_t day);
