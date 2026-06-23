#include "senzor_temperatura.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "app_state.h"

static const char *TAG = "TEMP_SENSOR";

#define TEMP_SENSOR_PERIOD_MS 3000  
#define DHT_LEVEL_TIMEOUT_US  100   
#define DHT_BIT_THRESHOLD_US  45    

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static uint16_t s_temp_tenths = 0;
static uint8_t  s_temp_negative = 0;
static uint16_t s_hum_tenths = 0;

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

esp_err_t temp_sensor_read(uint16_t *temp_tenths, uint8_t *temp_negative,
                           uint16_t *hum_tenths)
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

    uint16_t hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t temp = (((uint16_t)(data[2] & 0x7F)) << 8) | data[3];
    uint8_t neg = (data[2] & 0x80) ? 1 : 0;

    if (temp_tenths)   *temp_tenths = temp;
    if (temp_negative) *temp_negative = neg;
    if (hum_tenths)    *hum_tenths = hum;
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

void temp_sensor_task(void *pvParameters)
{
    (void) pvParameters;
    ESP_LOGI(TAG, "temp_sensor_task pornit");

    for (;;) {
        uint16_t temp = 0;
        uint8_t neg = 0;
        uint16_t hum = 0;
        esp_err_t err = temp_sensor_read(&temp, &neg, &hum);

        app_sensor_t snap = {0};
        if (err == ESP_OK) {
            s_temp_tenths = temp;
            s_temp_negative = neg;
            s_hum_tenths = hum;
            snap.temp_tenths = temp;
            snap.temp_negative = neg;
            snap.hum_tenths = hum;
            snap.valid = 1;
            ESP_LOGI(TAG, "Temp: %s%u.%u C | Umiditate: %u.%u %%RH",
                     neg ? "-" : "", temp / 10, temp % 10,
                     hum / 10, hum % 10);
        } else {
            snap.temp_tenths = s_temp_tenths;
            snap.temp_negative = s_temp_negative;
            snap.hum_tenths = s_hum_tenths;
            snap.valid = 0;
            ESP_LOGW(TAG, "Citire esuata: %s", esp_err_to_name(err));
        }
        app_state_set_sensor(&snap);

        vTaskDelay(pdMS_TO_TICKS(TEMP_SENSOR_PERIOD_MS));
    }
}

uint16_t temp_sensor_get_temp_tenths(void)  { return s_temp_tenths; }
uint8_t  temp_sensor_get_temp_negative(void) { return s_temp_negative; }
uint16_t temp_sensor_get_hum_tenths(void)   { return s_hum_tenths; }
