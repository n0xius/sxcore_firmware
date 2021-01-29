#pragma once

#include <gd32f3x0.h>

// values are saved as ARGB
#define LED_COLOR_OFF ((uint32_t)0x00000000)
#define LED_COLOR_RED ((uint32_t)0x00FF0000)
#define LED_COLOR_GREEN ((uint32_t)0x0000FF00)
#define LED_COLOR_BLUE ((uint32_t)0x000000FF)
#define LED_COLOR_PINK ((uint32_t)0x00FF00FF)
#define LED_COLOR_WHITE ((uint32_t)0x00FFFFFF)

extern uint8_t g_should_glow_led;
extern uint8_t g_cur_led_glow_value;
extern uint8_t g_should_fade_out_led_glow;

void set_led_color_red(uint8_t _red);
void set_led_color_green(uint8_t _green);
void set_led_color_blue(uint8_t _blue);
void set_led_color(uint32_t _color);

void toggle_blue_led_glow(uint8_t _should_glow);
void do_led_glow(void);
