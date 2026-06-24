#include "ui_screens.h"
#include "ui_topbar.h"
#include "ui_theme.h"
#include "ui_keyboard.h"

#include <stdint.h>
#include <string.h>

#include "lvgl.h"
#include "wifi_manager.h"
#include "rgb_led.h"
#include "settings_store.h"
#include "mqtt_comm.h"
#include "ota_update.h"

#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_idf_version.h"

#ifndef CURRENT_FW_VERSION
#define CURRENT_FW_VERSION "necunoscut"
#endif

typedef struct {
    lv_obj_t   *wifi_lbl;
    lv_obj_t   *mqtt_name_lbl;
    lv_obj_t   *heap_lbl;
    lv_obj_t   *uptime_lbl;
    lv_obj_t   *fw_lbl;
    lv_obj_t   *latest_lbl;
    lv_obj_t   *ota_status_lbl;
    lv_obj_t   *ota_btn;
    lv_timer_t *timer;
} settings_ui_t;

static wifi_scan_entry_t s_scan[WIFI_SCAN_MAX];
static uint8_t s_scan_n = 0;
static char s_sel_ssid[33];
static lv_obj_t *s_scan_modal = NULL;

/* ---- Nume utilizator ---- */

static void mqtt_name_done_cb(const char *text, void *user)
{
    settings_ui_t *ui = (settings_ui_t *)user;
    settings_set_mqtt_name(text);
    mqtt_comm_reconnect();
    if (ui != NULL && ui->mqtt_name_lbl != NULL) {
        char name[MQTT_DEVICE_NAME_LEN] = "";
        if (settings_get_mqtt_name(name, sizeof(name))) {
            lv_label_set_text_fmt(ui->mqtt_name_lbl, "Nume: %s", name);
        } else {
            lv_label_set_text(ui->mqtt_name_lbl, "Nume: (nesetat)");
        }
    }
}

static void mqtt_name_btn_cb(lv_event_t *e)
{
    settings_ui_t *ui = (settings_ui_t *)lv_event_get_user_data(e);
    char current[MQTT_DEVICE_NAME_LEN] = "";
    settings_get_mqtt_name(current, sizeof(current));
    ui_keyboard_show("Nume pe retea", current, mqtt_name_done_cb, ui);
}

static void password_done_cb(const char *text, void *user)
{
    (void) user;
    wifi_connect_to(s_sel_ssid, text);
}

static void close_scan_modal(void)
{
    if (s_scan_modal) {
        lv_obj_del(s_scan_modal);
        s_scan_modal = NULL;
    }
}

static void network_btn_cb(lv_event_t *e)
{
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    if (idx >= s_scan_n) {
        return;
    }
    strncpy(s_sel_ssid, s_scan[idx].ssid, sizeof(s_sel_ssid) - 1);
    s_sel_ssid[sizeof(s_sel_ssid) - 1] = '\0';
    uint8_t is_open = s_scan[idx].open;
    close_scan_modal();

    if (is_open) {
        wifi_connect_to(s_sel_ssid, "");
    } else {
        ui_keyboard_show("Parola WiFi", "", password_done_cb, NULL);
    }
}

static void scan_close_cb(lv_event_t *e)
{
    (void) e;
    close_scan_modal();
}

static void disconnect_btn_cb(lv_event_t *e)
{
    (void) e;
    wifi_disconnect();
}

static void reconnect_btn_cb(lv_event_t *e)
{
    (void) e;
    wifi_reconnect();
}

static void scan_btn_cb(lv_event_t *e)
{
    (void) e;
    s_scan_n = wifi_scan(s_scan, WIFI_SCAN_MAX);

    s_scan_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_scan_modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_radius(s_scan_modal, 0, 0);
    lv_obj_clear_flag(s_scan_modal, LV_OBJ_FLAG_SCROLLABLE);
    ui_style_dark(s_scan_modal);

    lv_obj_t *title = lv_label_create(s_scan_modal);
    lv_label_set_text_fmt(title, "Retele gasite: %u", s_scan_n);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *list = lv_list_create(s_scan_modal);
    lv_obj_set_size(list, LV_PCT(100), 230);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 28);

    for (uint8_t i = 0; i < s_scan_n; i++) {
        char line[48];
        lv_snprintf(line, sizeof(line), "%s (%u%%)%s", s_scan[i].ssid,
                    s_scan[i].strength, s_scan[i].open ? "" : " *");
        lv_obj_t *btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, line);
        lv_obj_add_event_cb(btn, network_btn_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);
    }

    lv_obj_t *close_btn = lv_btn_create(s_scan_modal);
    lv_obj_set_size(close_btn, LV_PCT(100), 36);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(close_btn, scan_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, "Inchide");
    lv_obj_center(close_lbl);
}

/* ---- Tema ---- */

static void theme_btn_cb(lv_event_t *e)
{
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    ui_theme_set(idx);
}

/* ---- LED (alegere culoare libera prin slidere R/G/B) ---- */

static lv_obj_t *s_led_modal = NULL;
static lv_obj_t *s_led_preview = NULL;
static lv_obj_t *s_sld_r = NULL;
static lv_obj_t *s_sld_g = NULL;
static lv_obj_t *s_sld_b = NULL;

static void close_led_modal(void)
{
    if (s_led_modal) {
        lv_obj_del(s_led_modal);
        s_led_modal = NULL;
        s_led_preview = NULL;
        s_sld_r = s_sld_g = s_sld_b = NULL;
    }
}

static void led_apply_from_sliders(bool save)
{
    if (!s_sld_r || !s_sld_g || !s_sld_b) {
        return;
    }
    uint8_t r = (uint8_t)lv_slider_get_value(s_sld_r);
    uint8_t g = (uint8_t)lv_slider_get_value(s_sld_g);
    uint8_t b = (uint8_t)lv_slider_get_value(s_sld_b);
    rgb_led_set(r, g, b);
    if (s_led_preview) {
        lv_obj_set_style_bg_color(s_led_preview, lv_color_make(r, g, b), 0);
    }
    if (save) {
        settings_set_led(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
    }
}

static void led_slider_cb(lv_event_t *e)
{
    led_apply_from_sliders(lv_event_get_code(e) == LV_EVENT_RELEASED);
}

static lv_obj_t *make_led_slider(lv_obj_t *parent, const char *name, uint8_t val)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), 40);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 10, 0);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, name);
    lv_obj_set_width(lbl, 22);

    lv_obj_t *sld = lv_slider_create(row);
    lv_obj_set_flex_grow(sld, 1);
    lv_slider_set_range(sld, 0, 255);
    lv_slider_set_value(sld, val, LV_ANIM_OFF);
    lv_obj_add_event_cb(sld, led_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(sld, led_slider_cb, LV_EVENT_RELEASED, NULL);
    return sld;
}

static void led_close_cb(lv_event_t *e)
{
    (void) e;
    close_led_modal();
}

static void led_open_cb(lv_event_t *e)
{
    (void) e;
    close_led_modal();

    uint32_t saved = settings_get_led();
    uint8_t r = (saved >> 16) & 0xFF;
    uint8_t g = (saved >> 8) & 0xFF;
    uint8_t b = saved & 0xFF;

    s_led_modal = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_led_modal, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_radius(s_led_modal, 0, 0);
    lv_obj_clear_flag(s_led_modal, LV_OBJ_FLAG_SCROLLABLE);
    ui_style_dark(s_led_modal);

    lv_obj_t *title = lv_label_create(s_led_modal);
    lv_label_set_text(title, "Culoare LED");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *cont = lv_obj_create(s_led_modal);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(92), 220);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 48);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, 12, 0);

    s_led_preview = lv_obj_create(cont);
    lv_obj_set_size(s_led_preview, LV_PCT(100), 56);
    lv_obj_set_style_radius(s_led_preview, 8, 0);
    lv_obj_set_style_border_width(s_led_preview, 1, 0);
    lv_obj_set_style_bg_opa(s_led_preview, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_led_preview, lv_color_make(r, g, b), 0);

    s_sld_r = make_led_slider(cont, "R", r);
    s_sld_g = make_led_slider(cont, "G", g);
    s_sld_b = make_led_slider(cont, "B", b);

    lv_obj_t *close_btn = lv_btn_create(s_led_modal);
    lv_obj_set_size(close_btn, LV_PCT(100), 42);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(close_btn, led_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, "Gata");
    lv_obj_center(close_lbl);
}

static void refresh_firmware_ui(settings_ui_t *ui)
{
    if (ui->fw_lbl) {
        lv_label_set_text_fmt(ui->fw_lbl, "Firmware: %s", CURRENT_FW_VERSION);
    }
    if (ui->latest_lbl) {
        lv_label_set_text_fmt(ui->latest_lbl, "Latest: %s", ota_get_latest_version());
    }
    if (ui->ota_btn) {
        if (ota_is_update_available() && ota_get_state() != OTA_STATE_DOWNLOADING) {
            lv_obj_clear_flag(ui->ota_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui->ota_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void ota_btn_cb(lv_event_t *e)
{
    (void) e;
    ota_update_start();
}

static void update_cb(lv_timer_t *timer)
{
    settings_ui_t *ui = (settings_ui_t *)timer->user_data;

    if (ui->wifi_lbl) {
        char ssid[33];
        char ip[16];
        wifi_get_ssid(ssid, sizeof(ssid));
        wifi_get_ip_string(ip, sizeof(ip));
        if (wifi_is_connected()) {
            lv_label_set_text_fmt(ui->wifi_lbl, "WiFi: %s\nIP: %s  Semnal: %u%%",
                                  ssid, ip, wifi_get_strength());
        } else if (wifi_has_saved()) {
            lv_label_set_text_fmt(ui->wifi_lbl, "WiFi: deconectat\nRetea salvata: %s", ssid);
        } else {
            lv_label_set_text(ui->wifi_lbl, "WiFi: deconectat\nNicio retea salvata");
        }
    }

    if (ui->mqtt_name_lbl) {
        char name[MQTT_DEVICE_NAME_LEN] = "";
        if (settings_get_mqtt_name(name, sizeof(name))) {
            lv_label_set_text_fmt(ui->mqtt_name_lbl, "Nume: %s", name);
        } else {
            lv_label_set_text(ui->mqtt_name_lbl, "Nume: (nesetat)");
        }
    }

    lv_label_set_text_fmt(ui->heap_lbl, "Heap liber: %u KB",
                          (unsigned)(esp_get_free_heap_size() / 1024));
    uint32_t up = (uint32_t)(esp_timer_get_time() / 1000000);
    lv_label_set_text_fmt(ui->uptime_lbl, "Uptime: %uh %um %us",
                          (unsigned)(up / 3600), (unsigned)((up % 3600) / 60),
                          (unsigned)(up % 60));

    if (ui->ota_status_lbl) {
        ota_state_t st = ota_get_state();
        if (st == OTA_STATE_IDLE) {
            lv_label_set_text(ui->ota_status_lbl, "");
        } else {
            lv_label_set_text(ui->ota_status_lbl, ota_get_status_text());
        }
    }

    refresh_firmware_ui(ui);
}

static void delete_cb(lv_event_t *e)
{
    settings_ui_t *ui = (settings_ui_t *)lv_event_get_user_data(e);
    if (ui->timer) {
        lv_timer_del(ui->timer);
    }
    lv_mem_free(ui);
}

static lv_obj_t *section_title(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    return lbl;
}

lv_obj_t *ui_settings_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *content = ui_topbar_create(scr, "Setari");
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);

    settings_ui_t *ui = lv_mem_alloc(sizeof(settings_ui_t));

    section_title(content, "Tema culoare");
    lv_obj_t *theme_row = lv_obj_create(content);
    lv_obj_remove_style_all(theme_row);
    lv_obj_set_size(theme_row, LV_PCT(100), 48);
    lv_obj_set_flex_flow(theme_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(theme_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(theme_row, 10, 0);
    for (uint8_t i = 0; i < ui_theme_count(); i++) {
        lv_obj_t *cbtn = lv_btn_create(theme_row);
        lv_obj_set_size(cbtn, 34, 34);
        lv_obj_set_style_radius(cbtn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(cbtn, ui_theme_color(i), 0);
        lv_obj_add_event_cb(cbtn, theme_btn_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);
    }

    section_title(content, "LED");
    lv_obj_t *led_btn = lv_btn_create(content);
    lv_obj_set_width(led_btn, LV_PCT(100));
    lv_obj_add_event_cb(led_btn, led_open_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *led_btn_lbl = lv_label_create(led_btn);
    lv_label_set_text(led_btn_lbl, LV_SYMBOL_TINT " Culoare LED");
    lv_obj_center(led_btn_lbl);

    section_title(content, "Numele Utilizatorului");
    ui->mqtt_name_lbl = lv_label_create(content);
    lv_label_set_text(ui->mqtt_name_lbl, "Nume: (nesetat)");
    lv_obj_t *mqtt_name_btn = lv_btn_create(content);
    lv_obj_set_width(mqtt_name_btn, LV_PCT(100));
    lv_obj_add_event_cb(mqtt_name_btn, mqtt_name_btn_cb, LV_EVENT_CLICKED, ui);
    lv_obj_t *mqtt_name_btn_lbl = lv_label_create(mqtt_name_btn);
    lv_label_set_text(mqtt_name_btn_lbl, LV_SYMBOL_EDIT " Seteaza numele");
    lv_obj_center(mqtt_name_btn_lbl);

    section_title(content, "WiFi");
    ui->wifi_lbl = lv_label_create(content);
    lv_label_set_text(ui->wifi_lbl, "WiFi: ...");
    lv_obj_t *scan_btn = lv_btn_create(content);
    lv_obj_set_width(scan_btn, LV_PCT(100));
    lv_obj_add_event_cb(scan_btn, scan_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *scan_lbl = lv_label_create(scan_btn);
    lv_label_set_text(scan_lbl, LV_SYMBOL_WIFI " Scaneaza retele");
    lv_obj_center(scan_lbl);

    lv_obj_t *wifi_btn_row = lv_obj_create(content);
    lv_obj_remove_style_all(wifi_btn_row);
    lv_obj_set_size(wifi_btn_row, LV_PCT(100), 44);
    lv_obj_set_flex_flow(wifi_btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wifi_btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *disc_btn = lv_btn_create(wifi_btn_row);
    lv_obj_set_width(disc_btn, LV_PCT(48));
    lv_obj_add_event_cb(disc_btn, disconnect_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *disc_lbl = lv_label_create(disc_btn);
    lv_label_set_text(disc_lbl, "Deconecteaza");
    lv_obj_center(disc_lbl);

    lv_obj_t *recon_btn = lv_btn_create(wifi_btn_row);
    lv_obj_set_width(recon_btn, LV_PCT(48));
    lv_obj_add_event_cb(recon_btn, reconnect_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *recon_lbl = lv_label_create(recon_btn);
    lv_label_set_text(recon_lbl, "Reconecteaza");
    lv_obj_center(recon_lbl);

    section_title(content, "Firmware");

    ui->fw_lbl = lv_label_create(content);
    lv_label_set_text_fmt(ui->fw_lbl, "Firmware: %s", CURRENT_FW_VERSION);

    ui->latest_lbl = lv_label_create(content);
    lv_label_set_text(ui->latest_lbl, "Latest: --");

    ui->ota_status_lbl = lv_label_create(content);
    lv_label_set_text(ui->ota_status_lbl, "");

    ui->ota_btn = lv_btn_create(content);
    lv_obj_set_width(ui->ota_btn, LV_PCT(100));
    lv_obj_add_flag(ui->ota_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(ui->ota_btn, ota_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ota_btn_lbl = lv_label_create(ui->ota_btn);
    lv_label_set_text(ota_btn_lbl, LV_SYMBOL_DOWNLOAD " Instaleaza update");
    lv_obj_center(ota_btn_lbl);

    ota_check_version();

    section_title(content, "Informatii sistem");

    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    lv_obj_t *chip_lbl = lv_label_create(content);
    lv_label_set_text_fmt(chip_lbl, "Chip: ESP32, %u nuclee", chip.cores);

    lv_obj_t *idf = lv_label_create(content);
    lv_label_set_text_fmt(idf, "ESP-IDF: %s", esp_get_idf_version());

    lv_obj_t *mac_lbl = lv_label_create(content);
    lv_label_set_text_fmt(mac_lbl, "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    ui->heap_lbl = lv_label_create(content);
    lv_label_set_text(ui->heap_lbl, "Heap liber: --");

    ui->uptime_lbl = lv_label_create(content);
    lv_label_set_text(ui->uptime_lbl, "Uptime: --");

    ui->timer = lv_timer_create(update_cb, 1000, ui);
    update_cb(ui->timer);

    lv_obj_add_event_cb(scr, delete_cb, LV_EVENT_DELETE, ui);

    return scr;
}
