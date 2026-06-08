#include "senzor_temperatura.h"

#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "TEMP_SENSOR";

#define TEMP_SENSOR_PERIOD_MS 3000  
#define DHT_LEVEL_TIMEOUT_US  100  
#define DHT_BIT_THRESHOLD_US  45  

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static float s_temperature = 0.0f;
static float s_humidity = 0.0f;

static int32_t dht_wait_level(int level, int32_t timeout_us)
{
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(TEMP_SENSOR_GPIO) != level) {
        if (esp_timer_get_time() - start > timeout_us) {
            return -1;
        }
    }
    return (int32_t)(esp_timer_get_time() - start);
}

esp_err_t temp_sensor_read(float *temperature, float *humidity)
{
    uint8_t data[5] = {0};

    gpio_set_direction(TEMP_SENSOR_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(TEMP_SENSOR_GPIO, 0);
    esp_rom_delay_us(1100);
    gpio_set_level(TEMP_SENSOR_GPIO, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(TEMP_SENSOR_GPIO, GPIO_MODE_INPUT);

    taskENTER_CRITICAL(&s_mux);

    if (dht_wait_level(0, DHT_LEVEL_TIMEOUT_US) < 0 ||
        dht_wait_level(1, DHT_LEVEL_TIMEOUT_US) < 0 ||
        dht_wait_level(0, DHT_LEVEL_TIMEOUT_US) < 0) {
        taskEXIT_CRITICAL(&s_mux);
        return ESP_ERR_TIMEOUT;
    }

    for (uint8_t i = 0; i < 40; i++) {
        if (dht_wait_level(1, DHT_LEVEL_TIMEOUT_US) < 0) {
            taskEXIT_CRITICAL(&s_mux);
            return ESP_ERR_TIMEOUT;
        }
        int32_t high_us = dht_wait_level(0, DHT_LEVEL_TIMEOUT_US);
        if (high_us < 0) {
            taskEXIT_CRITICAL(&s_mux);
            return ESP_ERR_TIMEOUT;
        }
        data[i / 8] <<= 1;
        if (high_us > DHT_BIT_THRESHOLD_US) {
            data[i / 8] |= 1;
        }
    }

    taskEXIT_CRITICAL(&s_mux);

    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return ESP_ERR_INVALID_CRC;
    }

    float hum = (float)(((uint16_t)data[0] << 8) | data[1]) * 0.1f;
    float temp = (float)((((uint16_t)(data[2] & 0x7F)) << 8) | data[3]) * 0.1f;
    if (data[2] & 0x80) {
        temp = -temp;
    }

    if (temperature) {
        *temperature = temp;
    }
    if (humidity) {
        *humidity = hum;
    }
    return ESP_OK;
}

void temp_sensor_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TEMP_SENSOR_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(TEMP_SENSOR_GPIO, 1);
    ESP_LOGI(TAG, "Senzor DHT22 initializat pe GPIO%d", TEMP_SENSOR_GPIO);
}
static float dht_dew_point(float t, float rh)
{
    const float a = 17.62f;
    const float b = 243.12f;
    float gamma = (a * t) / (b + t) + logf(rh / 100.0f);
    return (b * gamma) / (a - gamma);
}

static float dht_heat_index(float t_c, float rh)
{
    float t = t_c * 9.0f / 5.0f + 32.0f;
    float hi = -42.379f + 2.04901523f * t + 10.14333127f * rh
               - 0.22475541f * t * rh - 0.00683783f * t * t
               - 0.05481717f * rh * rh + 0.00122874f * t * t * rh
               + 0.00085282f * t * rh * rh - 0.00000199f * t * t * rh * rh;
    return (hi - 32.0f) * 5.0f / 9.0f;
}

void temp_sensor_task(void *pvParameters)
{
    (void) pvParameters;
    float temp = 0.0f;
    float hum = 0.0f;

    ESP_LOGI(TAG, "temp_sensor_task pornit");

    for (;;) {
        esp_err_t err = temp_sensor_read(&temp, &hum);
        if (err == ESP_OK) {
            s_temperature = temp;
            s_humidity = hum;
            float dew = dht_dew_point(temp, hum);
            float hi = dht_heat_index(temp, hum);
            ESP_LOGI(TAG,
                     "Temp: %.1f C | Umiditate: %.1f %%RH | Punct de roua: %.1f C | Resimtita: %.1f C",
                     temp, hum, dew, hi);
        } else {
            ESP_LOGW(TAG, "Citire esuata: %s", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(TEMP_SENSOR_PERIOD_MS));
    }
}

float temp_sensor_get_temperature(void)
{
    return s_temperature;
}

float temp_sensor_get_humidity(void)
{
    return s_humidity;
}