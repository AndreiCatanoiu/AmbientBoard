#include "ui_screens.h"
#include "ui_topbar.h"
#include "ui_keyboard.h"

#include "lvgl.h"
#include "mqtt_comm.h"
#include "inbox.h"
#include "settings_store.h"
#include "wifi_manager.h"

typedef struct {
    lv_obj_t   *list;
    lv_obj_t   *status;
    lv_obj_t   *write_btn;
    lv_timer_t *timer;
    uint32_t    last_version;
} messages_ui_t;

static void rebuild_list(messages_ui_t *ui)
{
    (void) ui;
    lv_obj_t *list = ui->list;
    lv_obj_clean(list);

    if (!mqtt_has_device_name()) {
        lv_obj_t *btn = lv_list_add_btn(list, NULL,
                                        "Seteaza numele in Setari");
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        return;
    }

    if (!wifi_is_connected()) {
        lv_obj_t *btn = lv_list_add_btn(list, NULL,
                                        "Conecteaza WiFi\npentru a vedea mesajele");
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        return;
    }

    uint8_t count = inbox_count();
    if (count == 0) {
        lv_obj_t *btn = lv_list_add_btn(list, NULL, "Niciun mesaj inca");
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        return;
    }

    char line[96];
    char from[INBOX_FROM_LEN];
    char body[INBOX_LINE_LEN];
    for (uint8_t i = 0; i < count; i++) {
        inbox_get(i, from, body, sizeof(body));
        lv_snprintf(line, sizeof(line), "%s: %s", from, body);
        lv_list_add_btn(list, NULL, line);
    }
}

static void update_cb(lv_timer_t *timer)
{
    messages_ui_t *ui = (messages_ui_t *)timer->user_data;

    bool connected = mqtt_is_connected();
    bool named = mqtt_has_device_name();

    if (connected && named) {
        lv_label_set_text(ui->status, "Broker: conectat");
    } else if (connected) {
        lv_label_set_text(ui->status, "Broker: conectat (fara nume)");
    } else {
        lv_label_set_text(ui->status, "Broker: deconectat");
    }

    if (ui->write_btn) {
        if (connected && named) {
            lv_obj_clear_state(ui->write_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(ui->write_btn, LV_STATE_DISABLED);
        }
    }

    static uint8_t fetch_ticks = 0;
    if (wifi_is_connected() && ++fetch_ticks >= 20) {
        fetch_ticks = 0;
        inbox_refresh();
    }

    uint32_t v = inbox_version();
    static bool last_connected = false;
    static bool last_named = false;
    if (v != ui->last_version || connected != last_connected || named != last_named) {
        ui->last_version = v;
        last_connected = connected;
        last_named = named;
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
    if (!mqtt_is_connected() || !mqtt_has_device_name()) {
        return;
    }
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

    ui->write_btn = lv_btn_create(content);
    lv_obj_set_size(ui->write_btn, LV_PCT(100), 40);
    lv_obj_align(ui->write_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(ui->write_btn, write_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *write_lbl = lv_label_create(ui->write_btn);
    lv_label_set_text(write_lbl, LV_SYMBOL_EDIT " Scrie mesaj");
    lv_obj_center(write_lbl);

    ui->timer = lv_timer_create(update_cb, 500, ui);
    update_cb(ui->timer);

    lv_obj_add_event_cb(scr, delete_cb, LV_EVENT_DELETE, ui);

    return scr;
}
