#pragma once

typedef enum {
    UI_SCREEN_HOME = 0,
    UI_SCREEN_MESSAGES,
    UI_SCREEN_SENSORS,
    UI_SCREEN_SETTINGS,
    UI_SCREEN_CALENDAR,
} ui_screen_t;

void ui_router_init(void);

void ui_router_show(ui_screen_t screen);
