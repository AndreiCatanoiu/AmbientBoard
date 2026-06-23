#pragma once

#include <stdint.h>
#include "lvgl.h"

#define UI_BG_COLOR   lv_color_hex(0x000000)
#define UI_TEXT_COLOR lv_color_white()

void ui_style_dark(lv_obj_t *obj);

uint8_t ui_theme_count(void);

const char *ui_theme_name(uint8_t idx);

lv_color_t ui_theme_color(uint8_t idx);

void ui_theme_apply_saved(void);

void ui_theme_set(uint8_t idx);

uint8_t ui_theme_current(void);
