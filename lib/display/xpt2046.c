#include "xpt2046.h"
#include "ili9341.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "XPT2046";

#define XPT2046_HOST SPI3_HOST
#define XPT2046_SPI_CLOCK_HZ (2 * 1000 * 1000)

#define XPT2046_CMD_X 0xD0
#define XPT2046_CMD_Y 0x90

#define XPT2046_SAMPLES 5

static spi_device_handle_t s_spi;

void xpt2046_init(void)
{
    gpio_config_t irq_conf = {
        .pin_bit_mask = (1ULL << XPT2046_PIN_IRQ),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&irq_conf);

    spi_bus_config_t buscfg = {
        .mosi_io_num = XPT2046_PIN_MOSI,
        .miso_io_num = XPT2046_PIN_MISO,
        .sclk_io_num = XPT2046_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(XPT2046_HOST, &buscfg, SPI_DMA_DISABLED));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = XPT2046_SPI_CLOCK_HZ,
        .mode = 0,
        .spics_io_num = XPT2046_PIN_CS,
        .queue_size = 2,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(XPT2046_HOST, &devcfg, &s_spi));

    ESP_LOGI(TAG, "XPT2046 initializat");
}

static uint16_t xpt2046_read_raw(uint8_t cmd)
{
    uint8_t tx[3] = {cmd, 0x00, 0x00};
    uint8_t rx[3] = {0};

    spi_transaction_t t = {0};
    t.length = 8 * 3;
    t.tx_buffer = tx;
    t.rx_buffer = rx;
    if (spi_device_polling_transmit(s_spi, &t) != ESP_OK) {
        return 0;
    }
    return (uint16_t)(((rx[1] << 8) | rx[2]) >> 3);
}

static uint16_t read_axis_avg(uint8_t cmd)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < XPT2046_SAMPLES; i++) {
        sum += xpt2046_read_raw(cmd);
    }
    return (uint16_t)(sum / XPT2046_SAMPLES);
}

static uint16_t map_axis(uint16_t raw, uint16_t out_max, uint8_t invert)
{
    if (raw < XPT2046_RAW_MIN) raw = XPT2046_RAW_MIN;
    if (raw > XPT2046_RAW_MAX) raw = XPT2046_RAW_MAX;
    int32_t v = (int32_t)(raw - XPT2046_RAW_MIN) * out_max /
                (XPT2046_RAW_MAX - XPT2046_RAW_MIN);
    if (invert) {
        v = out_max - v;
    }
    if (v < 0) v = 0;
    if (v > out_max) v = out_max;
    return (uint16_t)v;
}

bool xpt2046_read(uint16_t *x, uint16_t *y)
{
    if (gpio_get_level(XPT2046_PIN_IRQ) != 0) {
        return false;
    }

    uint16_t raw_x = read_axis_avg(XPT2046_CMD_X);
    uint16_t raw_y = read_axis_avg(XPT2046_CMD_Y);

    if (raw_x < XPT2046_RAW_MIN || raw_y < XPT2046_RAW_MIN) {
        return false;
    }

#if XPT2046_SWAP_XY
    uint16_t tmp = raw_x;
    raw_x = raw_y;
    raw_y = tmp;
#endif

    uint16_t px = map_axis(raw_x, ILI9341_WIDTH - 1, XPT2046_INVERT_X);
    uint16_t py = map_axis(raw_y, ILI9341_HEIGHT - 1, XPT2046_INVERT_Y);

#if XPT2046_DEBUG
    ESP_LOGI(TAG, "raw_x=%u raw_y=%u -> x=%u y=%u", raw_x, raw_y, px, py);
#endif

    if (x) *x = px;
    if (y) *y = py;
    return true;
}
