#include "app_state.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t s_mutex;
static app_sensor_t s_sensor;
static app_time_t   s_time;
static app_wifi_t   s_wifi;

void app_state_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    memset(&s_sensor, 0, sizeof(s_sensor));
    memset(&s_time, 0, sizeof(s_time));
    memset(&s_wifi, 0, sizeof(s_wifi));
}

static void lock(void)   { xSemaphoreTake(s_mutex, portMAX_DELAY); }
static void unlock(void) { xSemaphoreGive(s_mutex); }

void app_state_set_sensor(const app_sensor_t *s)
{
    lock();
    s_sensor = *s;
    unlock();
}

void app_state_get_sensor(app_sensor_t *out)
{
    lock();
    *out = s_sensor;
    unlock();
}

void app_state_set_time(const app_time_t *t)
{
    lock();
    s_time = *t;
    unlock();
}

void app_state_get_time(app_time_t *out)
{
    lock();
    *out = s_time;
    unlock();
}

void app_state_set_wifi(const app_wifi_t *w)
{
    lock();
    s_wifi = *w;
    unlock();
}

void app_state_get_wifi(app_wifi_t *out)
{
    lock();
    *out = s_wifi;
    unlock();
}
