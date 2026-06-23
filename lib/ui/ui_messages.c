#include "ui_screens.h"
#include "ui_topbar.h"
#include "ui_keyboard.h"

#include "lvgl.h"
#include "mqtt_comm.h"

typedef struct {
    lv_obj_t   *list;
    lv_obj_t   *status;
    lv_timer_t *timer;
    uint32_t    last_version;
} messages_ui_t;

static void rebuild_list(messages_ui_t *ui)
{
    lv_obj_clean(ui->list);
    uint8_t count = mqtt_get_message_count();
    if (count == 0) {
        lv_obj_t *btn = lv_list_add_btn(ui->list, NULL, "Niciun mesaj inca");
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    }
    for (uint8_t i = 0; i < count; i++) {
        char msg[MQTT_MSG_LEN];
        mqtt_get_message(i, msg, sizeof(msg));
        lv_list_add_btn(ui->list, NULL, msg);
    }
}

static void update_cb(lv_timer_t *timer)
{
    messages_ui_t *ui = (messages_ui_t *)timer->user_data;

    lv_label_set_text_fmt(ui->status, "Broker: %s",
                          mqtt_is_connected() ? "conectat" : "deconectat");

    uint32_t v = mqtt_get_version();
    if (v != ui->last_version) {
        ui->last_version = v;
        rebuild_list(ui);
    }
}

static void send_done_cb(const char *text, void *user)
{
    (void) user;
    mqtt_publish_message(text);
}

static void write_btn_cb(lv_event_t *e)
{
    (void) e;
    ui_keyboard_show("Scrie un mesaj", "", send_done_cb, NULL);
}

static void delete_cb(lv_event_t *e)
{
    messages_ui_t *ui = (messages_ui_t *)lv_event_get_user_data(e);
    if (ui->timer) {
        lv_timer_del(ui->timer);
    }
    lv_mem_free(ui);
}

lv_obj_t *ui_messages_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *content = ui_topbar_create(scr, "Mesaje");
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    messages_ui_t *ui = lv_mem_alloc(sizeof(messages_ui_t));
    ui->last_version = 0xFFFFFFFF;

    ui->status = lv_label_create(content);
    lv_obj_align(ui->status, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_text(ui->status, "Broker: ...");

    ui->list = lv_list_create(content);
    lv_obj_set_size(ui->list, LV_PCT(100), 170);
    lv_obj_align(ui->list, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_t *write_btn = lv_btn_create(content);
    lv_obj_set_size(write_btn, LV_PCT(100), 40);
    lv_obj_align(write_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(write_btn, write_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *write_lbl = lv_label_create(write_btn);
    lv_label_set_text(write_lbl, LV_SYMBOL_EDIT " Scrie mesaj");
    lv_obj_center(write_lbl);

    ui->timer = lv_timer_create(update_cb, 500, ui);
    update_cb(ui->timer);

    lv_obj_add_event_cb(scr, delete_cb, LV_EVENT_DELETE, ui);

    return scr;
}
