#include "ui_screens.h"
#include "ui_topbar.h"

#include "lvgl.h"
#include "app_state.h"

typedef struct {
    lv_obj_t   *temp;
    lv_obj_t   *hum;
    lv_timer_t *timer;
} sensors_ui_t;

static void update_cb(lv_timer_t *timer)
{
    sensors_ui_t *ui = (sensors_ui_t *)timer->user_data;

    app_sensor_t s;
    app_state_get_sensor(&s);

    lv_label_set_text_fmt(ui->temp, "Temperatura: %s%u.%u C",
                          s.temp_negative ? "-" : "",
                          s.temp_tenths / 10, s.temp_tenths % 10);
    lv_label_set_text_fmt(ui->hum, "Umiditate: %u.%u %%RH",
                          s.hum_tenths / 10, s.hum_tenths % 10);
}

static void delete_cb(lv_event_t *e)
{
    sensors_ui_t *ui = (sensors_ui_t *)lv_event_get_user_data(e);
    if (ui->timer) {
        lv_timer_del(ui->timer);
    }
    lv_mem_free(ui);
}

lv_obj_t *ui_sensors_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *content = ui_topbar_create(scr, "Temperatura");
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);

    sensors_ui_t *ui = lv_mem_alloc(sizeof(sensors_ui_t));

    ui->temp = lv_label_create(content);
    lv_obj_set_style_text_font(ui->temp, &lv_font_montserrat_20, 0);
    lv_label_set_text(ui->temp, "Temperatura: --");

    ui->hum = lv_label_create(content);
    lv_obj_set_style_text_font(ui->hum, &lv_font_montserrat_20, 0);
    lv_label_set_text(ui->hum, "Umiditate: --");

    ui->timer = lv_timer_create(update_cb, 1000, ui);
    update_cb(ui->timer);

    lv_obj_add_event_cb(scr, delete_cb, LV_EVENT_DELETE, ui);

    return scr;
}
