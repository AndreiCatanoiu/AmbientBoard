#include "ui_theme.h"
#include "settings_store.h"

typedef struct {
    const char *name;
    uint32_t    hex;
} ui_theme_def_t;

static const ui_theme_def_t s_themes[] = {
    {"Albastru Lime", 0x00B0FF},
    {"Rosu",          0xF44336},
    {"Verde Lime",    0x76FF03},
    {"Roz Bombon",    0xFF4081},
    {"Roz Inchis",    0xC2185B},
};

#define THEME_COUNT (sizeof(s_themes) / sizeof(s_themes[0]))

static uint8_t s_current = 0;

void ui_style_dark(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, UI_BG_COLOR, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(obj, UI_TEXT_COLOR, 0);
}

uint8_t ui_theme_count(void)
{
    return (uint8_t)THEME_COUNT;
}

const char *ui_theme_name(uint8_t idx)
{
    if (idx >= THEME_COUNT) idx = 0;
    return s_themes[idx].name;
}

lv_color_t ui_theme_color(uint8_t idx)
{
    if (idx >= THEME_COUNT) idx = 0;
    return lv_color_hex(s_themes[idx].hex);
}

uint8_t ui_theme_current(void)
{
    return s_current;
}

static void apply(uint8_t idx)
{
    if (idx >= THEME_COUNT) idx = 0;
    s_current = idx;

    lv_disp_t *disp = lv_disp_get_default();
    lv_theme_t *th = lv_theme_default_init(disp,
                                           lv_color_hex(s_themes[idx].hex),
                                           lv_palette_main(LV_PALETTE_GREY),
                                           true, /* mod intunecat */
                                           LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, th);
}

void ui_theme_apply_saved(void)
{
    apply(settings_get_theme());
}

void ui_theme_set(uint8_t idx)
{
    apply(idx);
    settings_set_theme(idx);
}
