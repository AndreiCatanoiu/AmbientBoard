#include "ui_screens.h"
#include "ui_topbar.h"

#include "lvgl.h"
#include "app_state.h"

typedef struct {
    lv_obj_t *cal;
} calendar_ui_t;

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

    uint16_t year = 2026;
    uint8_t month = 1;
    uint8_t day = 1;

    app_time_t t;
    app_state_get_time(&t);
    if (t.synced) {
        year = t.year;
        month = t.month;
        day = t.day;
    }

    ui->cal = lv_calendar_create(content);
    lv_obj_set_size(ui->cal, 230, LV_PCT(100));
    lv_obj_align(ui->cal, LV_ALIGN_TOP_MID, 0, 0);
    lv_calendar_set_today_date(ui->cal, year, month, day);
    lv_calendar_set_showed_date(ui->cal, year, month);

    lv_obj_add_event_cb(scr, delete_cb, LV_EVENT_DELETE, ui);

    return scr;
}
