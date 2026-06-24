#include "display.h"
#include "ili9341.h"
#include "xpt2046.h"
#include "rgb_led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_timer.h"
#include "esp_log.h"

#include "lvgl.h"

static const char *TAG = "DISPLAY";

#define LVGL_TICK_PERIOD_MS    2
#define LVGL_BUF_LINES         40
#define DISPLAY_SLEEP_MS       (3 * 60 * 1000)  

static SemaphoreHandle_t s_lvgl_mutex;
static lv_disp_draw_buf_t s_draw_buf;
static lv_color_t s_buf1[ILI9341_WIDTH * LVGL_BUF_LINES];
static lv_color_t s_buf2[ILI9341_WIDTH * LVGL_BUF_LINES];

static int64_t s_last_activity_us;
static bool s_backlight_on = true;
static bool s_wake_touch = false;

static void display_activity_touch(void)
{
    s_last_activity_us = esp_timer_get_time();
}

void display_wake(void)
{
    display_activity_touch();
    if (!s_backlight_on) {
        ili9341_backlight(1);
        s_backlight_on = true;
        s_wake_touch = true;
        rgb_led_resume();
    }
}

bool display_is_awake(void)
{
    return s_backlight_on;
}

static void display_sleep_check(void)
{
    if (!s_backlight_on) {
        return;
    }
    int64_t idle_us = esp_timer_get_time() - s_last_activity_us;
    if (idle_us >= (int64_t)DISPLAY_SLEEP_MS * 1000) {
        ili9341_backlight(0);
        s_backlight_on = false;
        rgb_led_suspend();
        ESP_LOGI(TAG, "Backlight stins (inactivitate %d s)", DISPLAY_SLEEP_MS / 1000);
    }
}

static void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    ili9341_flush(area->x1, area->y1, area->x2, area->y2, (const uint16_t *)color_p);
    lv_disp_flush_ready(drv);
}

static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void) drv;
    uint16_t x = 0;
    uint16_t y = 0;

    if (xpt2046_read(&x, &y)) {
        if (!s_backlight_on) {
            display_wake();
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        }
        if (s_wake_touch) {
            s_wake_touch = false;
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        }
        display_activity_touch();
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void lvgl_tick_cb(void *arg)
{
    (void) arg;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

bool display_lock(uint32_t timeout_ms)
{
    TickType_t ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(s_lvgl_mutex, ticks) == pdTRUE;
}

void display_unlock(void)
{
    xSemaphoreGiveRecursive(s_lvgl_mutex);
}

void display_init(void)
{
    ili9341_init();
    xpt2046_init();

    lv_init();

    lv_disp_draw_buf_init(&s_draw_buf, s_buf1, s_buf2,
                          ILI9341_WIDTH * LVGL_BUF_LINES);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = ILI9341_WIDTH;
    disp_drv.ver_res = ILI9341_HEIGHT;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &s_draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);

    const esp_timer_create_args_t tick_args = {
        .callback = &lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    s_lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    display_activity_touch();

    ESP_LOGI(TAG, "Display + LVGL initializate");
}

void display_task(void *pvParameters)
{
    (void) pvParameters;
    ESP_LOGI(TAG, "display_task pornit");
    for (;;) {
        display_sleep_check();
        if (display_lock(0)) {
            lv_timer_handler();
            display_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
