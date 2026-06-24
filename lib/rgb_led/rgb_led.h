#pragma once

#include <stdint.h>

#define RGB_LED_PIN_R 4
#define RGB_LED_PIN_G 16
#define RGB_LED_PIN_B 17

void rgb_led_init(void);

void rgb_led_set(uint8_t r, uint8_t g, uint8_t b);

void rgb_led_suspend(void);

void rgb_led_resume(void);
