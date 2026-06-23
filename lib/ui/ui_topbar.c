#include "ui_topbar.h"
#include "ui_router.h"
#include "ui_theme.h"

#define TOPBAR_HEIGHT 40

static void back_event_cb(lv_event_t *e)
{
    (void) e;
    ui_router_show(UI_SCREEN_HOME);
}

lv_obj_t *ui_topbar_create(lv_obj_t *parent, const char *title)
{
    ui_style_dark(parent);

    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, LV_PCT(100), TOPBAR_HEIGHT);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 4, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_text_color(bar, lv_color_white(), 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *back = lv_btn_create(bar);
    lv_obj_set_size(back, 40, 30);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(back, back_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT);
    lv_obj_center(back_lbl);

    lv_obj_t *title_lbl = lv_label_create(bar);
    lv_label_set_text(title_lbl, title);
    lv_obj_align(title_lbl, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *content = lv_obj_create(parent);
    lv_obj_set_size(content, LV_PCT(100), LV_VER_RES - TOPBAR_HEIGHT);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, TOPBAR_HEIGHT);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    ui_style_dark(content);

    return content;
}
