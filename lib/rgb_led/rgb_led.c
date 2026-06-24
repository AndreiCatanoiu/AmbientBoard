#include "rgb_led.h"

#include "settings_store.h"
#include "driver/ledc.h"

/* PWM pe 3 canale (R/G/B). LED-ul e cu anod comun, deci e activ pe LOW:
 * intensitate 0 -> pin mereu HIGH (stins), intensitate 255 -> pin mereu LOW. */
#define RGB_LEDC_MODE       LEDC_LOW_SPEED_MODE
#define RGB_LEDC_TIMER      LEDC_TIMER_0
#define RGB_LEDC_RES        LEDC_TIMER_8_BIT
#define RGB_LEDC_FREQ_HZ    5000
#define RGB_LEDC_DUTY_MAX   255

static const ledc_channel_t s_channels[3] = {
    LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2,
};
static const int s_pins[3] = {
    RGB_LED_PIN_R, RGB_LED_PIN_G, RGB_LED_PIN_B,
};

void rgb_led_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = RGB_LEDC_MODE,
        .timer_num       = RGB_LEDC_TIMER,
        .duty_resolution = RGB_LEDC_RES,
        .freq_hz         = RGB_LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    for (int i = 0; i < 3; i++) {
        ledc_channel_config_t ch = {
            .speed_mode = RGB_LEDC_MODE,
            .channel    = s_channels[i],
            .timer_sel  = RGB_LEDC_TIMER,
            .intr_type  = LEDC_INTR_DISABLE,
            .gpio_num   = s_pins[i],
            .duty       = RGB_LEDC_DUTY_MAX, /* stins (anod comun) */
            .hpoint     = 0,
        };
        ledc_channel_config(&ch);
    }

    rgb_led_set(0, 0, 0);
}

void rgb_led_set(uint8_t r, uint8_t g, uint8_t b)
{
    const uint8_t vals[3] = {r, g, b};
    for (int i = 0; i < 3; i++) {
        /* anod comun: duty inversat (0 = aprins maxim, 255 = stins) */
        uint32_t duty = RGB_LEDC_DUTY_MAX - vals[i];
        ledc_set_duty(RGB_LEDC_MODE, s_channels[i], duty);
        ledc_update_duty(RGB_LEDC_MODE, s_channels[i]);
    }
}

void rgb_led_suspend(void)
{
    rgb_led_set(0, 0, 0);
}

void rgb_led_resume(void)
{
    uint32_t led = settings_get_led();
    rgb_led_set((led >> 16) & 0xFF, (led >> 8) & 0xFF, led & 0xFF);
}
