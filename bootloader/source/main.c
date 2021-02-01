#include "gd32f3x0_libopt.h"
#include "gd32f3x0_it.h"
#include "gw_defines.h"

#include "usb.h"
#include "spi.h"
#include "delay.h"
#include "rgb_led.h"
#include "firmware_update.h"

extern uint8_t* __spi_buffer__;

void timer_initialize()
{
    gpio_af_set(GPIOB, GPIO_AF_2, GPIO_PIN_8);
    gpio_af_set(GPIOB, GPIO_AF_2, GPIO_PIN_9);
    gpio_af_set(GPIOA, GPIO_AF_2, GPIO_PIN_10);

    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_8);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_9);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_10);

    timer_deinit(TIMER0);
    timer_deinit(TIMER15);
    timer_deinit(TIMER16);

    timer_parameter_struct timer_parameter;

    //timer_parameter.prescaler = 107; // kHz - 1 / 1000000 or MHz - 1?
    timer_parameter.prescaler = rcu_clock_freq_get(CK_SYS) / 1000000;
    timer_parameter.alignedmode = TIMER_COUNTER_EDGE;
    timer_parameter.clockdivision = TIMER_CKDIV_DIV1;
    timer_parameter.period = 255;
    timer_parameter.repetitioncounter = 0;

    timer_init(TIMER0, &timer_parameter);
    timer_init(TIMER15, &timer_parameter);
    timer_init(TIMER16, &timer_parameter);

    timer_oc_parameter_struct timer_oc_param;

    timer_oc_param.outputstate = TIMER_CCX_ENABLE;
    timer_oc_param.outputnstate = TIMER_CCXN_ENABLE;
    timer_oc_param.ocpolarity = TIMER_OCN_POLARITY_HIGH;
    timer_oc_param.ocnpolarity = TIMER_OCN_POLARITY_LOW;
    timer_oc_param.ocidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_oc_param.ocnidlestate = TIMER_OCN_IDLE_STATE_HIGH;

    timer_channel_output_config(TIMER15, TIMER_CH_0, &timer_oc_param);
    timer_channel_output_pulse_value_config(TIMER15, TIMER_CH_0, 256);
    timer_channel_output_shadow_config(TIMER15, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_fast_config(TIMER15, TIMER_CH_0, TIMER_OC_FAST_DISABLE);
    timer_automatic_output_enable(TIMER15);
    timer_auto_reload_shadow_enable(TIMER15);

    timer_channel_output_config(TIMER16, TIMER_CH_0, &timer_oc_param);
    timer_channel_output_pulse_value_config(TIMER16, TIMER_CH_0, 256);
    timer_channel_output_shadow_config(TIMER16, TIMER_CH_0, TIMER_OC_MODE_PWM0);
    timer_channel_output_fast_config(TIMER16, TIMER_CH_0, TIMER_OC_FAST_DISABLE);
    timer_automatic_output_enable(TIMER16);
    timer_auto_reload_shadow_enable(TIMER16);

    timer_channel_output_config(TIMER0, TIMER_CH_2, &timer_oc_param);
    timer_channel_output_pulse_value_config(TIMER0, TIMER_CH_2, 256);
    timer_channel_output_shadow_config(TIMER0, TIMER_CH_2, TIMER_OC_MODE_PWM0);
    timer_channel_output_fast_config(TIMER0, TIMER_CH_2, TIMER_OC_FAST_DISABLE);
    timer_automatic_output_enable(TIMER0);
    timer_auto_reload_shadow_enable(TIMER0);

    timer_enable(TIMER0);
    timer_enable(TIMER16);
    timer_enable(TIMER15);
}

void hardware_initialize()
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

    // NOTE: the usb clock initialization is duplicated for whatever reason?!
    {
        // NOTE: replaced "usbfs_prescaler = RCU_USBFS_CKPLL_DIV2" with proper code below for support of dynamic clocks
        switch(rcu_clock_freq_get(CK_SYS))
        {
            case 48000000U:
                usbfs_prescaler = RCU_USBFS_CKPLL_DIV1;
                break;
            case 72000000U:
                usbfs_prescaler = RCU_USBFS_CKPLL_DIV1_5;
                break;
            case 96000000U:
                usbfs_prescaler = RCU_USBFS_CKPLL_DIV2;
                break;
            default:
                break;
        }

        rcu_usbfs_clock_config(usbfs_prescaler);

        rcu_periph_clock_enable(RCU_USBFS);

        rcu_periph_clock_enable(RCU_PMU);
    }

    rcu_periph_clock_enable(RCU_USART0);

    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);

    rcu_periph_clock_enable(RCU_TIMER0);
    rcu_periph_clock_enable(RCU_TIMER13);
    rcu_periph_clock_enable(RCU_TIMER14);
    rcu_periph_clock_enable(RCU_TIMER15);
    rcu_periph_clock_enable(RCU_TIMER16);

    timer_initialize();

    delay_init();
}

void initialize_vector_table()
{
    nvic_vector_table_set((uint32_t)&__firmware, 0);

    // call reset_handler inside firmware
    ((reset_function_t)((uint32_t*)__firmware)[1])();
}

void shutdown()
{
    set_led_color(LED_COLOR_OFF);
    gpiob_turn_off_pin10();

    while ( 1 )
        pmu_to_standbymode(0);
}

void handle_spi_commands(void)
{

    spi0_send_fpga_cmd(1);

    while (1)
    {
        while (1)
        {
            // NOTE: polling for data?
            while ((spi0_recv_0B_via_26() & 0x10) == 0);

            spi0_send_05_recv_BA(1, __spi_buffer__, 512);

            spi0_send_fpga_cmd(5);

            uint32_t status_code = 0;

            switch(__spi_buffer__[0])
            {
                case 0x22:
                {
                    status_code = validate_firmware_header((uint8_t*)__spi_buffer__ + 16, __spi_buffer__[1]);
                    break;
                }

                case 0x23:
                {
                    for(uint32_t i = 0; i != 0x1D0; i += 64)
                    {
                        status_code = handle_firmware_update(((uint8_t*)__spi_buffer__ + 16) + i);

                        if (status_code != GW_STATUS_RECEIVED_UPDATE_BLOCK)
                            break;
                    }

                    break;
                }

                case 0x24:
                {
                    uint32_t color = REG32((uint32_t)__spi_buffer__ + 16);
                    set_led_color(color);
                    status_code = GW_STATUS_SUCCESS;
                    break;
                }

                case 0x25:
                {
                    shutdown();
                    break;
                }

                case 0x26:
                {
                    uint32_t* g_debug_mode = (uint32_t*)((uint32_t)&__bootloader + 0x168);
                    uint32_t* g_flash_function_table = (uint32_t*)((uint32_t)&__firmware + 0x150);
                    uint32_t* g_did_initialize_functions = (uint32_t*)((uint32_t)&__firmware + 0x1FC);

                    if (*g_debug_mode != BOOTLOADER_DEBUG_MODE && *g_did_initialize_functions)
                        break;

                    g_ram_function_table = g_flash_function_table;
                    status_code = GW_STATUS_RESET;
                    break;
                }

                default:
                    // how did we end up here?
                    break;
            }

            *(uint32_t*)__spi_buffer__ = status_code;
            spi0_send_05_send_BC(1, __spi_buffer__, 512);
            spi0_send_fpga_cmd(3);

            // NOTE: not a firmware update command, go back into the first while loop
            if (__spi_buffer__[0] != 0x22 && __spi_buffer__[0] != 0x23)
                break;
        }
    }
}

int main(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);

    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_9);
    FlagStatus usb_vbus = gpio_input_bit_get(GPIOA, GPIO_PIN_9);

    uint32_t* g_debug_mode = (uint32_t*)((uint32_t)&__bootloader + 0x168);
    uint32_t* g_did_initialize_functions = (uint32_t*)((uint32_t)&__firmware + 0x1FC);

    // check if usb vbus is present
    if ((*g_debug_mode == BOOTLOADER_DEBUG_MODE || !*g_did_initialize_functions) && usb_vbus == RESET)
        initialize_vector_table();

    hardware_initialize();

    set_led_color(LED_COLOR_GREEN);

    handle_usb_transfers();
    return 0;
}