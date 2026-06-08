#include "time_manager.h"

#include <time.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_sntp.h"

#include "wifi_manager.h"

static const char *TAG = "TIME_MGR";

#define TIME_LOG_PERIOD_MS   10000
#define TIME_SYNC_TIMEOUT_MS 15000
#define TIME_YEAR_VALID      (2020 - 1900)

static bool s_synced = false;

static struct tm time_now(void)
{
    time_t now = 0;
    struct tm timeinfo = { 0 };
    time(&now);
    localtime_r(&now, &timeinfo);
    return timeinfo;
}

void time_init(void)
{
    setenv("TZ", TIME_TZ_ROMANIA, 1);
    tzset();
    ESP_LOGI(TAG, "Fus orar setat: %s", TIME_TZ_ROMANIA);
}

bool time_is_synced(void)
{
    return s_synced;
}

static void time_start_sntp(void)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, TIME_NTP_SERVER);
    esp_sntp_init();
    ESP_LOGI(TAG, "SNTP pornit (server: %s)", TIME_NTP_SERVER);
}

static bool time_wait_for_sync(void)
{
    const TickType_t step = pdMS_TO_TICKS(1000);
    uint32_t elapsed_ms = 0;

    while (elapsed_ms < TIME_SYNC_TIMEOUT_MS) {
        struct tm timeinfo = time_now();
        if (timeinfo.tm_year >= TIME_YEAR_VALID) {
            return true;
        }
        ESP_LOGI(TAG, "Astept sincronizarea ceasului...");
        vTaskDelay(step);
        elapsed_ms += 1000;
    }
    return false;
}

void time_task(void *pvParameters)
{
    (void) pvParameters;

    ESP_LOGI(TAG, "time_task pornit, astept conexiunea WiFi");
    while (!wifi_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "WiFi conectat, sincronizez ceasul");

    time_start_sntp();

    while (!time_wait_for_sync()) {
        ESP_LOGW(TAG, "Sincronizare esuata, reincerc");
    }
    s_synced = true;
    ESP_LOGI(TAG, "Ceas sincronizat: %s %02d:%02d:%02d",
             time_get_date_string(), time_get_hour(),
             time_get_minute(), time_get_second());

    for (;;) {
        struct tm t = time_now();
        ESP_LOGI(TAG, "Data: %02d-%02d-%04d  Ora: %02d:%02d:%02d",
                 t.tm_mday, t.tm_mon + 1, t.tm_year + 1900,
                 t.tm_hour, t.tm_min, t.tm_sec);
        vTaskDelay(pdMS_TO_TICKS(TIME_LOG_PERIOD_MS));
    }
}

uint8_t  time_get_day(void)    { return (uint8_t)time_now().tm_mday; }
uint8_t  time_get_month(void)  { return (uint8_t)(time_now().tm_mon + 1); }
uint16_t time_get_year(void)   { return (uint16_t)(time_now().tm_year + 1900); }
uint8_t  time_get_hour(void)   { return (uint8_t)time_now().tm_hour; }
uint8_t  time_get_minute(void) { return (uint8_t)time_now().tm_min; }
uint8_t  time_get_second(void) { return (uint8_t)time_now().tm_sec; }

char *time_get_date_string(void)
{
    static char buf[36];
    struct tm t = time_now();
    snprintf(buf, sizeof(buf), "%02d - %02d - %04d",
             t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
    return buf;
}