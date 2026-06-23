#include "ui_screens.h"
#include "ui_topbar.h"
#include "ui_keyboard.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "lvgl.h"
#include "app_state.h"
#include "settings_store.h"

typedef struct {
    lv_obj_t *cal;
    lv_obj_t *events_lbl;
    uint16_t  year;
    uint8_t   month;
    uint8_t   sel_day;
} calendar_ui_t;

static lv_calendar_date_t s_highlights[CAL_MAX_EVENTS];
static calendar_event_t   s_events[CAL_MAX_EVENTS];

static void refresh_highlights(calendar_ui_t *ui)
{
    uint8_t total = settings_calendar_load(s_events, CAL_MAX_EVENTS);
    uint8_t n = 0;
    for (uint8_t i = 0; i < total && n < CAL_MAX_EVENTS; i++) {
        if (s_events[i].year == ui->year && s_events[i].month == ui->month) {
            s_highlights[n].year = s_events[i].year;
            s_highlights[n].month = s_events[i].month;
            s_highlights[n].day = s_events[i].day;
            n++;
        }
    }
    lv_calendar_set_highlighted_dates(ui->cal, s_highlights, n);
}

static void refresh_events_label(calendar_ui_t *ui)
{
    char buf[256];
    int pos = snprintf(buf, sizeof(buf), "Evenimente %02u.%02u:\n",
                       ui->sel_day, ui->month);

    uint8_t total = settings_calendar_load(s_events, CAL_MAX_EVENTS);
    uint8_t shown = 0;
    for (uint8_t i = 0; i < total; i++) {
        if (s_events[i].year == ui->year && s_events[i].month == ui->month &&
            s_events[i].day == ui->sel_day) {
            int w = snprintf(buf + pos, (size_t)(sizeof(buf) - pos), "- %s\n",
                             s_events[i].text);
            if (w > 0) {
                pos += w;
            }
            shown++;
            if (pos >= (int)sizeof(buf) - 1) {
                break;
            }
        }
    }
    if (shown == 0) {
        snprintf(buf + pos, sizeof(buf) - (size_t)pos, "(niciunul)");
    }
    lv_label_set_text(ui->events_lbl, buf);
}

static void add_event_done_cb(const char *text, void *user)
{
    calendar_ui_t *ui = (calendar_ui_t *)user;
    if (text == NULL || text[0] == '\0') {
        return;
    }
    calendar_event_t ev = {0};
    ev.year = ui->year;
    ev.month = ui->month;
    ev.day = ui->sel_day;
    strncpy(ev.text, text, CAL_TEXT_LEN - 1);
    settings_calendar_add(&ev);

    refresh_highlights(ui);
    refresh_events_label(ui);
}

static void add_btn_cb(lv_event_t *e)
{
    calendar_ui_t *ui = (calendar_ui_t *)lv_event_get_user_data(e);
    char title[48];
    snprintf(title, sizeof(title), "Eveniment %02u.%02u.%04u",
             ui->sel_day, ui->month, ui->year);
    ui_keyboard_show(title, "", add_event_done_cb, ui);
}

static void calendar_event_cb(lv_event_t *e)
{
    calendar_ui_t *ui = (calendar_ui_t *)lv_event_get_user_data(e);
    lv_obj_t *cal = lv_event_get_target(e);

    lv_obj_t *btnm = lv_calendar_get_btnmatrix(cal);
    uint16_t btn_id = lv_btnmatrix_get_selected_btn(btnm);
    if (btn_id < 7) {
        return;
    }

    lv_calendar_date_t d;
    if (lv_calendar_get_pressed_date(cal, &d) != LV_RES_OK) {
        return;
    }
    if (d.day < 1 || d.day > 31) {
        return;
    }

    ui->year = (uint16_t)d.year;
    ui->month = (uint8_t)d.month;
    ui->sel_day = (uint8_t)d.day;
    refresh_highlights(ui);
    refresh_events_label(ui);
}

static void delete_cb(lv_event_t *e)
{
    calendar_ui_t *ui = (calendar_ui_t *)lv_event_get_user_data(e);
    lv_mem_free(ui);
}

lv_obj_t *ui_calendar_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *content = ui_topbar_create(scr, "Calendar");
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    calendar_ui_t *ui = lv_mem_alloc(sizeof(calendar_ui_t));
    if (ui == NULL) {
        return scr;
    }

    app_time_t t;
    app_state_get_time(&t);
    if (t.synced) {
        ui->year = t.year;
        ui->month = t.month;
        ui->sel_day = t.day;
    } else {
        ui->year = 2026;
        ui->month = 1;
        ui->sel_day = 1;
    }

    ui->cal = lv_calendar_create(content);
    lv_obj_set_size(ui->cal, 230, 200);
    lv_obj_align(ui->cal, LV_ALIGN_TOP_MID, 0, 0);
    lv_calendar_set_today_date(ui->cal, ui->year, ui->month, ui->sel_day);
    lv_calendar_set_showed_date(ui->cal, ui->year, ui->month);
    lv_obj_add_event_cb(ui->cal, calendar_event_cb, LV_EVENT_VALUE_CHANGED, ui);

    ui->events_lbl = lv_label_create(content);
    lv_obj_set_width(ui->events_lbl, LV_PCT(100));
    lv_obj_align(ui->events_lbl, LV_ALIGN_TOP_MID, 0, 204);
    lv_label_set_text(ui->events_lbl, "");

    lv_obj_t *add_btn = lv_btn_create(content);
    lv_obj_set_size(add_btn, LV_PCT(100), 36);
    lv_obj_align(add_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(add_btn, add_btn_cb, LV_EVENT_CLICKED, ui);
    lv_obj_t *add_lbl = lv_label_create(add_btn);
    lv_label_set_text(add_lbl, LV_SYMBOL_PLUS " Adauga eveniment");
    lv_obj_center(add_lbl);

    refresh_highlights(ui);
    refresh_events_label(ui);

    lv_obj_add_event_cb(scr, delete_cb, LV_EVENT_DELETE, ui);

    return scr;
}
