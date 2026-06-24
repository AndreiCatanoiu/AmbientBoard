#include "ui_router.h"
#include "ui_screens.h"

#include "lvgl.h"

static lv_obj_t *s_home = NULL;
static bool s_on_home = true;

static lv_obj_t *create_screen(ui_screen_t screen)
{
    switch (screen) {
    case UI_SCREEN_MESSAGES: return ui_messages_create();
    case UI_SCREEN_SENSORS:  return ui_sensors_create();
    case UI_SCREEN_SETTINGS: return ui_settings_create();
    case UI_SCREEN_CALENDAR: return ui_calendar_create();
    default:                 return s_home;
    }
}

void ui_router_show(ui_screen_t screen)
{
    if (screen > UI_SCREEN_CALENDAR) {
        return;
    }

    lv_obj_t *target = (screen == UI_SCREEN_HOME) ? s_home : create_screen(screen);

    bool auto_del = !s_on_home;
    lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, auto_del);

    s_on_home = (screen == UI_SCREEN_HOME);
}

void ui_router_init(void)
{
    s_home = ui_home_create();
    s_on_home = true;
    lv_scr_load(s_home);
}
