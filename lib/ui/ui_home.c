#include "ui_screens.h"
#include "ui_router.h"
#include "ui_theme.h"

#include <stdint.h>

#include "lvgl.h"
#include "app_state.h"

static const char *WEEKDAYS[] = {
    "Duminica", "Luni", "Marti", "Miercuri", "Joi", "Vineri", "Sambata"
};

static lv_obj_t *s_time_lbl;
static lv_obj_t *s_date_lbl;
static lv_obj_t *s_weekday_lbl;
static lv_timer_t *s_timer;

static uint8_t weekday_of(uint16_t y, uint8_t m, uint8_t d)
{
    static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) {
        y -= 1;
    }
    return (uint8_t)((y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7);
}

static void update_cb(lv_timer_t *timer)
{
    (void) timer;

    app_time_t t;
    app_state_get_time(&t);

    lv_label_set_text_fmt(s_time_lbl, "%02u:%02u", t.hour, t.minute);

    if (t.synced) {
        lv_label_set_text_fmt(s_date_lbl, "%02u.%02u.%04u", t.day, t.month, t.year);
        lv_label_set_text(s_weekday_lbl, WEEKDAYS[weekday_of(t.year, t.month, t.day)]);
    } else {
        lv_label_set_text(s_date_lbl, "--.--.----");
        lv_label_set_text(s_weekday_lbl, "Sincronizare...");
    }
}

static void app_btn_cb(lv_event_t *e)
{
    ui_screen_t scr = (ui_screen_t)(uintptr_t)lv_event_get_user_data(e);
    ui_router_show(scr);
}

static lv_obj_t *make_app_btn(lv_obj_t *parent, const char *symbol,
                              const char *text, ui_screen_t screen)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 104, 70);
    lv_obj_add_event_cb(btn, app_btn_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)screen);

    lv_obj_t *col = lv_obj_create(btn);
    lv_obj_remove_style_all(col);
    lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *icon = lv_label_create(col);
    lv_label_set_text(icon, symbol);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);

    lv_obj_t *lbl = lv_label_create(col);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_width(lbl, LV_PCT(100));
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);

    return btn;
}

lv_obj_t *ui_home_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    ui_style_dark(scr);

    s_time_lbl = lv_label_create(scr);
    lv_obj_set_style_text_font(s_time_lbl, &lv_font_montserrat_48, 0);
    lv_label_set_text(s_time_lbl, "00:00");
    lv_obj_align(s_time_lbl, LV_ALIGN_TOP_MID, 0, 14);

    s_weekday_lbl = lv_label_create(scr);
    lv_obj_set_style_text_font(s_weekday_lbl, &lv_font_montserrat_20, 0);
    lv_label_set_text(s_weekday_lbl, "...");
    lv_obj_align(s_weekday_lbl, LV_ALIGN_TOP_MID, 0, 74);

    s_date_lbl = lv_label_create(scr);
    lv_obj_set_style_text_font(s_date_lbl, &lv_font_montserrat_16, 0);
    lv_label_set_text(s_date_lbl, "--.--.----");
    lv_obj_align(s_date_lbl, LV_ALIGN_TOP_MID, 0, 102);

    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, 232, 172);
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(grid, 16, 0);
    lv_obj_set_style_pad_column(grid, 16, 0);

    make_app_btn(grid, LV_SYMBOL_ENVELOPE, "Mesaje", UI_SCREEN_MESSAGES);
    make_app_btn(grid, LV_SYMBOL_CHARGE, "Temperatura", UI_SCREEN_SENSORS);
    make_app_btn(grid, LV_SYMBOL_SETTINGS, "Setari", UI_SCREEN_SETTINGS);
    make_app_btn(grid, LV_SYMBOL_LIST, "Calendar", UI_SCREEN_CALENDAR);

    if (s_timer == NULL) {
        s_timer = lv_timer_create(update_cb, 500, NULL);
    }
    update_cb(NULL);

    return scr;
}
