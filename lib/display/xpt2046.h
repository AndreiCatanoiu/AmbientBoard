#pragma once

#include <stdint.h>
#include <stdbool.h>

#define XPT2046_PIN_MOSI 32
#define XPT2046_PIN_MISO 39
#define XPT2046_PIN_SCLK 25
#define XPT2046_PIN_CS   33
#define XPT2046_PIN_IRQ  36

#define XPT2046_DEBUG 0

#define XPT2046_SWAP_XY 0
#define XPT2046_INVERT_X 1
#define XPT2046_INVERT_Y 0

#define XPT2046_RAW_MIN 240
#define XPT2046_RAW_MAX 3800

void xpt2046_init(void);

bool xpt2046_read(uint16_t *x, uint16_t *y);
