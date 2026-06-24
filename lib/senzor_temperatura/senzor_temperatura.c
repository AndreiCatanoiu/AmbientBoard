#include "senzor_temperatura.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

#include "app_state.h"

static const char *TAG = "TEMP_SENSOR";

#define TEMP_SENSOR_PERIOD_MS   3000
#define DHT_START_LOW_US        1800
#define DHT_START_RELEASE_US    40
#define DHT_RESPONSE_TIMEOUT_US 100
#define DHT_BIT_TIMEOUT_US      100
#define DHT_BIT1_MIN_US         40

#define CCOUNT_MHZ              240
#define TEMP_FILTER_SAMPLES     5
#define TEMP_MIN_TENTHS         0
#define TEMP_MAX_TENTHS         600
#define TEMP_OUTLIER_TENTHS     25

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static uint16_t s_temp_tenths = 0;
static uint8_t  s_temp_negative = 0;
static uint16_t s_hum_tenths = 0;

static int32_t s_temp_hist_c[TEMP_FILTER_SAMPLES];
static uint16_t s_hum_hist[TEMP_FILTER_SAMPLES];
static uint8_t s_hist_len = 0;
static uint8_t s_hist_idx = 0;

static uint32_t IRAM_ATTR cpu_ccount(void)
{
    uint32_t c;
    __asm__ __volatile__("rsr %0, ccount" : "=a"(c));
    return c;
}

static int IRAM_ATTR dht_wait_level_us(int level, int timeout_us)
{
    uint32_t start = cpu_ccount();
    uint32_t limit = (uint32_t)timeout_us * CCOUNT_MHZ;
    while (gpio_get_level(TEMP_SENSOR_GPIO) != level) {
        if ((cpu_ccount() - start) > limit) {
            return -1;
        }
    }
    return (int)((cpu_ccount() - start) / CCOUNT_MHZ);
}

static uint16_t median_u16(uint16_t *v, uint8_t n)
{
    uint16_t tmp[TEMP_FILTER_SAMPLES];
    for (uint8_t i = 0; i < n; i++) {
        tmp[i] = v[i];
    }
    for (uint8_t i = 0; i < n; i++) {
        for (uint8_t j = i + 1; j < n; j++) {
            if (tmp[j] < tmp[i]) {
                uint16_t t = tmp[i];
                tmp[i] = tmp[j];
                tmp[j] = t;
            }
        }
    }
    return tmp[n / 2];
}

static int32_t median_i32(int32_t *v, uint8_t n)
{
    int32_t tmp[TEMP_FILTER_SAMPLES];
    for (uint8_t i = 0; i < n; i++) {
        tmp[i] = v[i];
    }
    for (uint8_t i = 0; i < n; i++) {
        for (uint8_t j = i + 1; j < n; j++) {
            if (tmp[j] < tmp[i]) {
                int32_t t = tmp[i];
                tmp[i] = tmp[j];
                tmp[j] = t;
            }
        }
    }
    return tmp[n / 2];
}

static void filter_get(int32_t *temp_c, uint16_t *hum)
{
    if (s_hist_len == 0) {
        return;
    }
    int32_t tc[TEMP_FILTER_SAMPLES];
    uint16_t hh[TEMP_FILTER_SAMPLES];
    for (uint8_t i = 0; i < s_hist_len; i++) {
        uint8_t idx = (uint8_t)((s_hist_idx + TEMP_FILTER_SAMPLES - s_hist_len + i) %
                                TEMP_FILTER_SAMPLES);
        tc[i] = s_temp_hist_c[idx];
        hh[i] = s_hum_hist[idx];
    }
    *temp_c = median_i32(tc, s_hist_len);
    *hum = median_u16(hh, s_hist_len);
}

static bool filter_push(int32_t temp_c, uint16_t hum)
{
    if (s_hist_len >= 2) {
        int32_t cur = temp_c;
        uint16_t dummy = hum;
        filter_get(&cur, &dummy);
        if (cur > 0 && (temp_c > cur + TEMP_OUTLIER_TENTHS ||
                        temp_c < cur - TEMP_OUTLIER_TENTHS)) {
            ESP_LOGW(TAG, "Citire suspecta %ld zecimi, ignor", (long)temp_c);
            return false;
        }
    }

    s_temp_hist_c[s_hist_idx] = temp_c;
    s_hum_hist[s_hist_idx] = hum;
    s_hist_idx = (s_hist_idx + 1) % TEMP_FILTER_SAMPLES;
    if (s_hist_len < TEMP_FILTER_SAMPLES) {
        s_hist_len++;
    }
    return true;
}

esp_err_t temp_sensor_read(uint16_t *temp_tenths, uint8_t *temp_negative,
                           uint16_t *hum_tenths)
{
    uint8_t data[5] = {0};

    gpio_set_direction(TEMP_SENSOR_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(TEMP_SENSOR_GPIO, 0);
    esp_rom_delay_us(DHT_START_LOW_US);
    gpio_set_level(TEMP_SENSOR_GPIO, 1);
    esp_rom_delay_us(DHT_START_RELEASE_US);
    gpio_set_direction(TEMP_SENSOR_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(TEMP_SENSOR_GPIO, GPIO_PULLUP_ONLY);

    portENTER_CRITICAL(&s_mux);

    if (dht_wait_level_us(0, DHT_RESPONSE_TIMEOUT_US) < 0 ||
        dht_wait_level_us(1, DHT_RESPONSE_TIMEOUT_US) < 0 ||
        dht_wait_level_us(0, DHT_RESPONSE_TIMEOUT_US) < 0) {
        portEXIT_CRITICAL(&s_mux);
        return ESP_ERR_TIMEOUT;
    }

    for (uint8_t i = 0; i < 40; i++) {
        if (dht_wait_level_us(1, DHT_BIT_TIMEOUT_US) < 0) {
            portEXIT_CRITICAL(&s_mux);
            return ESP_ERR_TIMEOUT;
        }
        int32_t high_us = dht_wait_level_us(0, DHT_BIT_TIMEOUT_US);
        if (high_us < 0) {
            portEXIT_CRITICAL(&s_mux);
            return ESP_ERR_TIMEOUT;
        }
        data[i / 8] <<= 1;
        if (high_us > DHT_BIT1_MIN_US) {
            data[i / 8] |= 1;
        }
    }

    portEXIT_CRITICAL(&s_mux);

    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return ESP_ERR_INVALID_CRC;
    }

    uint16_t hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t temp = (((uint16_t)(data[2] & 0x7F)) << 8) | data[3];
    uint8_t neg = (data[2] & 0x80) ? 1 : 0;

    if (temp_tenths) {
        *temp_tenths = temp;
    }
    if (temp_negative) {
        *temp_negative = neg;
    }
    if (hum_tenths) {
        *hum_tenths = hum;
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
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "DHT22 pe GPIO%d", TEMP_SENSOR_GPIO);
}

void temp_sensor_task(void *pvParameters)
{
    (void) pvParameters;
    ESP_LOGI(TAG, "temp_sensor_task pe core %d", xPortGetCoreID());

    for (;;) {
        uint16_t raw_temp = 0;
        uint8_t raw_neg = 0;
        uint16_t raw_hum = 0;
        esp_err_t err = temp_sensor_read(&raw_temp, &raw_neg, &raw_hum);

        app_sensor_t snap = {0};
        if (err == ESP_OK) {
            uint8_t out_neg = raw_neg;
            int32_t temp_c = (int32_t)raw_temp;

            if (temp_c >= TEMP_MIN_TENTHS && temp_c <= TEMP_MAX_TENTHS &&
                filter_push(temp_c, raw_hum)) {
                int32_t filt_c = temp_c;
                uint16_t filt_hum = raw_hum;
                filter_get(&filt_c, &filt_hum);

                s_temp_tenths = (uint16_t)filt_c;
                s_temp_negative = out_neg;
                s_hum_tenths = filt_hum;

                snap.temp_tenths = s_temp_tenths;
                snap.temp_negative = s_temp_negative;
                snap.hum_tenths = s_hum_tenths;
                snap.valid = 1;

                ESP_LOGI(TAG, "DHT %s%u.%u C | %u.%u %%RH",
                         snap.temp_negative ? "-" : "",
                         snap.temp_tenths / 10, snap.temp_tenths % 10,
                         snap.hum_tenths / 10, snap.hum_tenths % 10);
            } else {
                snap.temp_tenths = s_temp_tenths;
                snap.temp_negative = s_temp_negative;
                snap.hum_tenths = s_hum_tenths;
                snap.valid = (s_hist_len > 0);
            }
        } else {
            snap.temp_tenths = s_temp_tenths;
            snap.temp_negative = s_temp_negative;
            snap.hum_tenths = s_hum_tenths;
            snap.valid = (s_hist_len > 0);
            ESP_LOGW(TAG, "Citire esuata: %s", esp_err_to_name(err));
        }
        app_state_set_sensor(&snap);

        vTaskDelay(pdMS_TO_TICKS(TEMP_SENSOR_PERIOD_MS));
    }
}

uint16_t temp_sensor_get_temp_tenths(void)
{
    return s_temp_tenths;
}

uint8_t temp_sensor_get_temp_negative(void)
{
    return s_temp_negative;
}

uint16_t temp_sensor_get_hum_tenths(void)
{
    return s_hum_tenths;
}
