#include "ui_keyboard.h"
#include "ui_theme.h"

#include "lvgl.h"

static ui_keyboard_done_cb s_cb = NULL;
static void *s_user = NULL;
static lv_obj_t *s_modal = NULL;
static lv_obj_t *s_ta = NULL;

#define KB_HEIGHT_PX  158

static void close_modal(void)
{
    if (s_modal) {
        lv_obj_del(s_modal);
        s_modal = NULL;
        s_ta = NULL;
    }
}

static void kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        const char *txt = lv_textarea_get_text(s_ta);
        ui_keyboard_done_cb cb = s_cb;
        void *user = s_user;
        char buf[128];
        lv_snprintf(buf, sizeof(buf), "%s", txt);
        close_modal();
        if (cb) {
            cb(buf, user);
        }
    } else if (code == LV_EVENT_CANCEL) {
        close_modal();
    }
}

static void style_phone_keyboard(lv_obj_t *kb)
{
    lv_obj_set_style_pad_row(kb, 5, LV_PART_ITEMS);
    lv_obj_set_style_pad_column(kb, 6, LV_PART_ITEMS);
    lv_obj_set_style_pad_top(kb, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(kb, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_left(kb, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_right(kb, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(kb, 5, LV_PART_ITEMS);
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_16, LV_PART_ITEMS);
}

void ui_keyboard_show(const char *title, const char *initial,
                      ui_keyboard_done_cb cb, void *user)
{
    close_modal();
    s_cb = cb;
    s_user = user;

    s_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_radius(s_modal, 0, 0);
    lv_obj_set_style_pad_all(s_modal, 0, 0);
    lv_obj_clear_flag(s_modal, LV_OBJ_FLAG_SCROLLABLE);
    ui_style_dark(s_modal);

    lv_obj_t *title_lbl = lv_label_create(s_modal);
    lv_label_set_text(title_lbl, title ? title : "Introdu text");
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(title_lbl, LV_ALIGN_TOP_MID, 0, 4);

    s_ta = lv_textarea_create(s_modal);
    lv_textarea_set_one_line(s_ta, true);
    lv_textarea_set_text(s_ta, initial ? initial : "");
    lv_obj_set_style_text_font(s_ta, &lv_font_montserrat_16, 0);
    lv_obj_set_size(s_ta, LV_PCT(96), 34);
    lv_obj_align(s_ta, LV_ALIGN_TOP_MID, 0, 22);

    lv_obj_t *kb = lv_keyboard_create(s_modal);
    lv_obj_set_size(kb, LV_PCT(100), KB_HEIGHT_PX);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    style_phone_keyboard(kb);
    lv_keyboard_set_textarea(kb, s_ta);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_CANCEL, NULL);
}
