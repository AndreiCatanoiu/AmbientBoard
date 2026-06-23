#pragma once

#include <stdint.h>

#define ILI9341_WIDTH  240
#define ILI9341_HEIGHT 320

#define DISPLAY_USE_ST7789 1

#ifndef ILI9341_MADCTL
#  if DISPLAY_USE_ST7789
#    define ILI9341_MADCTL 0x00
#  else
#    define ILI9341_MADCTL 0x48
#  endif
#endif

#ifndef ILI9341_INVERT
#  if DISPLAY_USE_ST7789
#    define ILI9341_INVERT 0
#  else
#    define ILI9341_INVERT 1
#  endif
#endif

#define ILI9341_PIN_MOSI 13
#define ILI9341_PIN_MISO 12
#define ILI9341_PIN_SCLK 14
#define ILI9341_PIN_CS   15
#define ILI9341_PIN_DC   2
#define ILI9341_PIN_BL   21

void ili9341_init(void);

void ili9341_flush(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                   const uint16_t *color_data);

void ili9341_backlight(uint8_t on);
