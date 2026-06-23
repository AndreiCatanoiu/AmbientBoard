#include "ili9341.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

static const char *TAG = "ILI9341";

#define ILI9341_HOST SPI2_HOST
#define ILI9341_SPI_CLOCK_HZ (20 * 1000 * 1000)

static spi_device_handle_t s_spi;

typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t len;   
} lcd_init_cmd_t;

static const lcd_init_cmd_t s_init_cmds_ili9341[] = {
    {0xEF, {0x03, 0x80, 0x02}, 3},
    {0xCF, {0x00, 0xC1, 0x30}, 3},
    {0xED, {0x64, 0x03, 0x12, 0x81}, 4},
    {0xE8, {0x85, 0x00, 0x78}, 3},
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
    {0xF7, {0x20}, 1},
    {0xEA, {0x00, 0x00}, 2},
    {0xC0, {0x23}, 1},               
    {0xC1, {0x10}, 1},
    {0xC5, {0x3E, 0x28}, 2},         
    {0xC7, {0x86}, 1},
    {0x36, {ILI9341_MADCTL}, 1},     
    {0x3A, {0x55}, 1},              
    {ILI9341_INVERT ? 0x21 : 0x20, {0}, 0}, 
    {0xB1, {0x00, 0x18}, 2},
    {0xB6, {0x08, 0x82, 0x27}, 3},   
    {0xF2, {0x00}, 1},
    {0x26, {0x01}, 1},               
    {0xE0, {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}, 15},
    {0xE1, {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}, 15},
    {0x11, {0}, 0xFF},               
    {0x29, {0}, 0xFF},              
    {0x00, {0}, 0},                  
};

static const lcd_init_cmd_t s_init_cmds_st7789[] = {
    {0x11, {0}, 0xFF},               
    {0x36, {ILI9341_MADCTL}, 1},    
    {0x3A, {0x55}, 1},               
    {ILI9341_INVERT ? 0x21 : 0x20, {0}, 0}, 
    {0xB2, {0x0C, 0x0C, 0x00, 0x33, 0x33}, 5}, 
    {0xB7, {0x35}, 1},              
    {0xBB, {0x19}, 1},             
    {0xC0, {0x2C}, 1},               
    {0xC2, {0x01}, 1},              
    {0xC3, {0x12}, 1},               
    {0xC4, {0x20}, 1},               
    {0xC6, {0x0F}, 1},               
    {0xD0, {0xA4, 0xA1}, 2},        
    {0xE0, {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23}, 14},
    {0xE1, {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23}, 14},
    {0x29, {0}, 0xFF},              
    {0x00, {0}, 0},                 
};

static void ili9341_cmd(uint8_t cmd)
{
    gpio_set_level(ILI9341_PIN_DC, 0);
    spi_transaction_t t = {0};
    t.length = 8;
    t.tx_buffer = &cmd;
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi, &t));
}

static void ili9341_data(const uint8_t *data, int len)
{
    if (len <= 0) {
        return;
    }
    gpio_set_level(ILI9341_PIN_DC, 1);
    spi_transaction_t t = {0};
    t.length = 8 * len;
    t.tx_buffer = data;
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi, &t));
}

void ili9341_backlight(uint8_t on)
{
    gpio_set_level(ILI9341_PIN_BL, on ? 1 : 0);
}

void ili9341_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ILI9341_PIN_DC) | (1ULL << ILI9341_PIN_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    spi_bus_config_t buscfg = {
        .mosi_io_num = ILI9341_PIN_MOSI,
        .miso_io_num = ILI9341_PIN_MISO,
        .sclk_io_num = ILI9341_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = ILI9341_WIDTH * 40 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(ILI9341_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = ILI9341_SPI_CLOCK_HZ,
        .mode = 0,
        .spics_io_num = ILI9341_PIN_CS,
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(ILI9341_HOST, &devcfg, &s_spi));

    ili9341_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(150));

    const lcd_init_cmd_t *cmds =
        DISPLAY_USE_ST7789 ? s_init_cmds_st7789 : s_init_cmds_ili9341;

    for (int i = 0; cmds[i].len != 0 || cmds[i].cmd != 0; i++) {
        ili9341_cmd(cmds[i].cmd);
        if (cmds[i].len == 0xFF) {
            vTaskDelay(pdMS_TO_TICKS(120));
        } else {
            ili9341_data(cmds[i].data, cmds[i].len);
        }
    }

    ili9341_backlight(1);
    ESP_LOGI(TAG, "Display initializat (%s)",
             DISPLAY_USE_ST7789 ? "ST7789" : "ILI9341");
}

void ili9341_flush(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                   const uint16_t *color_data)
{
    uint8_t buf[4];

    ili9341_cmd(0x2A); 
    buf[0] = x1 >> 8;  buf[1] = x1 & 0xFF;
    buf[2] = x2 >> 8;  buf[3] = x2 & 0xFF;
    ili9341_data(buf, 4);

    ili9341_cmd(0x2B); 
    buf[0] = y1 >> 8;  buf[1] = y1 & 0xFF;
    buf[2] = y2 >> 8;  buf[3] = y2 & 0xFF;
    ili9341_data(buf, 4);

    ili9341_cmd(0x2C); 

    uint32_t pixels = (uint32_t)(x2 - x1 + 1) * (y2 - y1 + 1);
    gpio_set_level(ILI9341_PIN_DC, 1);
    spi_transaction_t t = {0};
    t.length = pixels * 16;
    t.tx_buffer = color_data;
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi, &t));
}
