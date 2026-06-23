#include "settings_store.h"

#include <string.h>

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "SETTINGS";
static const char *NS = "amb";

#define KEY_THEME    "theme"
#define KEY_SSID     "wifi_ssid"
#define KEY_PASS     "wifi_pass"
#define KEY_CAL_CNT  "cal_cnt"
#define KEY_CAL_BLOB "cal_blob"
#define KEY_LED      "led_rgb"

void settings_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initializat");
}

uint8_t settings_get_theme(void)
{
    nvs_handle_t h;
    uint8_t val = 0;
    if (nvs_open(NS, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u8(h, KEY_THEME, &val);
        nvs_close(h);
    }
    return val;
}

void settings_set_theme(uint8_t idx)
{
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, KEY_THEME, idx);
        nvs_commit(h);
        nvs_close(h);
    }
}

uint32_t settings_get_led(void)
{
    nvs_handle_t h;
    uint32_t val = 0;
    if (nvs_open(NS, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u32(h, KEY_LED, &val);
        nvs_close(h);
    }
    return val;
}

void settings_set_led(uint32_t rgb)
{
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u32(h, KEY_LED, rgb);
        nvs_commit(h);
        nvs_close(h);
    }
}

bool settings_get_wifi(char *ssid, char *pass)
{
    nvs_handle_t h;
    bool ok = false;
    if (nvs_open(NS, NVS_READONLY, &h) == ESP_OK) {
        size_t ssid_len = 33;
        size_t pass_len = 65;
        if (nvs_get_str(h, KEY_SSID, ssid, &ssid_len) == ESP_OK &&
            nvs_get_str(h, KEY_PASS, pass, &pass_len) == ESP_OK) {
            ok = (ssid[0] != '\0');
        }
        nvs_close(h);
    }
    return ok;
}

void settings_set_wifi(const char *ssid, const char *pass)
{
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, KEY_SSID, ssid);
        nvs_set_str(h, KEY_PASS, pass);
        nvs_commit(h);
        nvs_close(h);
        ESP_LOGI(TAG, "Credentiale WiFi salvate (SSID: %s)", ssid);
    }
}

static uint8_t cal_load(calendar_event_t *events)
{
    nvs_handle_t h;
    uint8_t count = 0;
    if (nvs_open(NS, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u8(h, KEY_CAL_CNT, &count);
        if (count > CAL_MAX_EVENTS) {
            count = CAL_MAX_EVENTS;
        }
        if (count > 0 && events != NULL) {
            size_t size = count * sizeof(calendar_event_t);
            if (nvs_get_blob(h, KEY_CAL_BLOB, events, &size) != ESP_OK) {
                count = 0;
            }
        }
        nvs_close(h);
    }
    return count;
}

uint8_t settings_calendar_count(void)
{
    return cal_load(NULL);
}

uint8_t settings_calendar_load(calendar_event_t *events, uint8_t max)
{
    if (events == NULL || max == 0) {
        return 0;
    }
    uint8_t count = cal_load(events);
    if (count > max) {
        count = max;
    }
    return count;
}

bool settings_calendar_get(uint8_t index, calendar_event_t *out)
{
    calendar_event_t events[CAL_MAX_EVENTS];
    uint8_t count = cal_load(events);
    if (index >= count) {
        return false;
    }
    *out = events[index];
    return true;
}

bool settings_calendar_add(const calendar_event_t *ev)
{
    calendar_event_t events[CAL_MAX_EVENTS];
    uint8_t count = cal_load(events);
    if (count >= CAL_MAX_EVENTS) {
        return false;
    }
    events[count] = *ev;
    count++;

    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) != ESP_OK) {
        return false;
    }
    nvs_set_blob(h, KEY_CAL_BLOB, events, count * sizeof(calendar_event_t));
    nvs_set_u8(h, KEY_CAL_CNT, count);
    nvs_commit(h);
    nvs_close(h);
    return true;
}

uint8_t settings_calendar_count_for_day(uint16_t year, uint8_t month, uint8_t day)
{
    calendar_event_t events[CAL_MAX_EVENTS];
    uint8_t count = cal_load(events);
    uint8_t matches = 0;
    for (uint8_t i = 0; i < count; i++) {
        if (events[i].year == year && events[i].month == month &&
            events[i].day == day) {
            matches++;
        }
    }
    return matches;
}
