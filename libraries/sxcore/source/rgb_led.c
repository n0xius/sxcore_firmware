#include "rgb_led.h"

uint8_t g_should_glow_led = 0;
uint8_t g_cur_led_glow_value = 0;
uint8_t g_should_fade_out_led_glow = 0;

void set_led_color_red(uint8_t _red)
{
    timer_channel_output_pulse_value_config(TIMER15, TIMER_CH_0, 256 - _red);
}

void set_led_color_green(uint8_t _green)
{
    timer_channel_output_pulse_value_config(TIMER16, TIMER_CH_0, 256 - _green);
}

void set_led_color_blue(uint8_t _blue)
{
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_2, 256 - _blue);
}

void set_led_color(uint32_t _color)
{
    set_led_color_red((_color >> 16) & 0xFF);
    set_led_color_green((_color >> 8) & 0xFF);
    set_led_color_blue(_color & 0xFF);
}

void toggle_blue_led_glow(uint8_t _should_glow)
{
    if ( g_should_glow_led == _should_glow )
        return;

    g_should_glow_led = _should_glow;
    g_should_fade_out_led_glow = 0;
    g_cur_led_glow_value = 0;

    timer_deinit(TIMER13);

    if ( _should_glow != 1 )
        return;

    timer_parameter_struct timer_param;

    timer_param.prescaler = 7;
    timer_param.alignedmode = 0;
    timer_param.counterdirection = 0;
    timer_param.clockdivision = 0;
    timer_param.repetitioncounter = 0;
    timer_param.period = 16383;

    timer_init(TIMER13, &timer_param);
    timer_update_event_enable(TIMER13);
    timer_interrupt_enable(TIMER13, TIMER_INT_UP);
    timer_interrupt_flag_clear(TIMER13, TIMER_INT_FLAG_UP);
    timer_update_source_config(TIMER13, TIMER_UPDATE_SRC_GLOBAL);
    timer_enable(TIMER13);
}

void do_led_glow(void)
{
    if ( timer_flag_get(TIMER13, TIMER_FLAG_UP) != SET )
        return;

    timer_interrupt_flag_clear(TIMER13, TIMER_INT_FLAG_UP);

    if ( g_should_glow_led != 1 )
        return;

    if ( g_should_fade_out_led_glow )
    {
        if ( !--g_cur_led_glow_value )
            g_should_fade_out_led_glow = 0;
    }
    else
    {
        if ( ++g_cur_led_glow_value == 255 )
            g_should_fade_out_led_glow = 1;
    }

    set_led_color_red(0);
    set_led_color_green(0);
    set_led_color_blue(g_cur_led_glow_value);
}