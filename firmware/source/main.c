#include "gd32f3x0_libopt.h"
#include "gd32f3x0_it.h"

#include "gw_defines.h"
#include "tea.h"
#include "spi.h"
#include "emmc.h"
#include "delay.h"
#include "flash.h"
#include "glitch.h"
#include "rgb_led.h"
#include "diagnostic.h"
#include "configuration.h"

extern uint32_t usbfs_prescaler;

const uint8_t g_aes_keys[16] = {
    0x7B, 0xC7, 0x6D, 0x76,
    0xFF, 0x45, 0xEA, 0xFA,
    0x0C, 0x3B, 0x73, 0x0A,
    0xCC, 0x25, 0x93, 0x00
};

uint8_t g_block_buffer[512];
uint8_t spi_buffer[512];

bootloader_usb_s* g_usb = 0;

typedef struct _timeout_s
{
    uint32_t update_time;
    uint32_t dword4;
    uint64_t total_time_passed;
} timeout_s;

void timeout_initialize(timeout_s* _timeout)
{
    _timeout->update_time = SysTick->VAL;
    _timeout->total_time_passed = 0;
}

void timeout_update(timeout_s* _timeout)
{
    int current_time = SysTick->VAL;

    _timeout->total_time_passed += (_timeout->update_time - current_time) & 0xFFFFFF;
    _timeout->update_time = current_time;
}

int timeout_did_reach_timeout_ms(timeout_s* _timeout, uint32_t _ms)
{
    return _timeout->total_time_passed >= ((uint64_t)96000) * _ms;
}

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

    timer_parameter.prescaler = 107; // = (CPU Clock MHz) - 1
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

    // setup_timer13_irq
    nvic_irq_enable(TIMER13_IRQn, 1u, 1u);
}

void setup_chip_model_pins(void)
{
    gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_3);
    gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_5);
}

void hardware_initialize()
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

    // NOTE: the usb clock initialization is duplicated for whatever reason?!
    {
        uint32_t usbfs_prescaler = 0;

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

        rcu_periph_reset_enable(RCU_USBFSRST);

        rcu_periph_reset_enable(RCU_PMURST);
    }

    rcu_periph_reset_enable(RCU_USART0RST);

    rcu_periph_reset_enable(RCU_GPIOARST);
    rcu_periph_reset_enable(RCU_GPIOBRST);
    rcu_periph_reset_enable(RCU_GPIOFRST);

    rcu_periph_reset_enable(RCU_SPI0RST);

    rcu_periph_reset_enable(RCU_TIMER0RST);
    rcu_periph_reset_enable(RCU_TIMER13RST);
    rcu_periph_reset_enable(RCU_TIMER14RST);
    rcu_periph_reset_enable(RCU_TIMER15RST);
    rcu_periph_reset_enable(RCU_TIMER16RST);

    // setup_usart0
    {
        // doesn't work since on the GD32F350Cx, GPIO_AF_0 on pin 6 = I2C0_SCL and on pin 7 = I2C0_SDA

        // USART0_TX
        gpio_af_set(GPIOB, GPIO_AF_0, GPIO_PIN_6);
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_6);
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_6);

        // USART0_RX
        gpio_af_set(GPIOB, GPIO_AF_0, GPIO_PIN_7);
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_7);
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_7);

        usart_deinit(USART0);
        usart_baudrate_set(USART0, 115200);
        usart_receive_config(USART0, USART_RECEIVE_ENABLE);
        usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
        usart_enable(USART0);
    }

    timer_initialize();

    // setup_ckout
    {
        rcu_ckout_config(RCU_CKOUTSRC_CKPLL_DIV2, RCU_CKOUT_DIV1);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_8);
        gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_8);
    }

    // setup_spi0
    {
        initialize_spi0(0);                           // SPI_PSC_2

        gpio_bit_reset(GPIOB, GPIO_PIN_10);
        gpio_bit_set(GPIOA, GPIO_PIN_4);
        gpio_bit_set(GPIOA, GPIO_PIN_5);
        gpio_bit_set(GPIOA, GPIO_PIN_7);
        gpio_bit_set(GPIOA, GPIO_PIN_6);

        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);

        gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_5);
        gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_6);
        gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_7);

        gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_10);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_5);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_6);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_7);
        gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_1);
        gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_4);
    }

    setup_chip_model_pins();

    // setup_gpiof_pin6
    {
        gpio_mode_set(GPIOF, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_6);
    }

    delay_init();
}

void shutdown()
{
    set_led_color(LED_COLOR_OFF);
    gpiob_turn_off_pin10();

    while ( 1 )
        pmu_to_standbymode(0);
}

void set_start_addr_to_firmware()
{
    nvic_vector_table_set(__firmware, 0);
    hardware_initialize();
}

extern uint32_t __data_start__;
extern uint32_t __data_size__;
extern uint32_t __bss_start__;
extern uint32_t __bss_size__;

void firmware_reset()
{
    // NOTE: copy data into ram
    memcpy(&__bss_start__, &__data_start__, (uint32_t)&__data_size__);

    // NOTE: clear stack
    memset(&__bss_start__, 0, (uint32_t)&__bss_size__);

    set_start_addr_to_firmware();
}

void handle_usb_command(bootloader_usb_s* _usb, uint32_t _usb_cmd_size, uint8_t _is_authenticated)
{
    if (_usb_cmd_size <= 3)
        return;

    uint32_t cmd = REG32(_usb->recv_buffer);
    uint32_t status = 0;

    switch(cmd)
    {
        case CMD_PING:
        {
            status = GW_STATUS_SUCCESS;
            *(uint32_t*)(_usb->send_buffer) = status;
            break;
        }

        case CMD_SPI0_INITALIZATION:
        {
            status = spi0_init_with_psc_4();

            *(uint32_t*)(_usb->send_buffer) = status;
            break;
        }

        case CMD_RESET_FPGA:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            spi0_send_fpga_cmd(0x80);
            delay_ms(100);
            spi0_send_fpga_cmd(0);

            if ( REG32((uint32_t)(_usb->recv_buffer + 4)) == 0xAABBCCDD )
            {
                delay_ms(100);
                spi0_send_fpga_cmd(0x40);
                delay_ms(2000);
                spi0_send_fpga_cmd(0);
                delay_ms(1);
            }

            status = GW_STATUS_SUCCESS;
            break;
        }

        case CMD_SEND_FPGA_CMD:
        case CMD_SEND_FPGA_CMD_1:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint32_t fpga_cmd = REG32((uint32_t)(_usb->recv_buffer + 4));

            spi0_send_fpga_cmd(fpga_cmd);

            status = GW_STATUS_SUCCESS;
            break;
        }

        case CMD_SEND_MMC_CMD:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint32_t mmc_cmd = REG32((uint32_t)(_usb->recv_buffer + 4));
            uint32_t mmc_arg = REG32((uint32_t)(_usb->recv_buffer + 8));
            mmc_send_command(mmc_cmd, mmc_arg, _usb->send_buffer, 0);
            break;
        }

        case CMD_WRITE_MMC_BLOCK:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint32_t block_index = REG32((uint32_t)(_usb->recv_buffer + 4)) & 15;

            memcpy((uint8_t*)(g_block_buffer + block_index * 32), (uint8_t*)(_usb->recv_buffer + 8), 32);

            if ( block_index == 15 )
            {
                spi0_send_05_via_24(1);
                spi0_send_data_BC(g_block_buffer, 512);
            }

            status = GW_STATUS_SUCCESS;
            break;
        }

        case CMD_READ_MMC_BLOCK:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint32_t block_index = REG32((uint32_t)(_usb->recv_buffer + 4)) & 15;

            if ( block_index == 0 )
            {
                spi0_send_05_via_24(1);
                spi0_recv_data_BA(g_block_buffer, 512);
            }

            memcpy((uint8_t*)(_usb->send_buffer + 8), (uint8_t*)(g_block_buffer + block_index * 32), 32);
            break;
        }

        case CMD_COPY_MMC_BLOCK:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint32_t block_index = REG32((uint32_t)(_usb->recv_buffer + 4)) & 15;

            if ( block_index == 0 )
                spi0_recv_data_BA(g_block_buffer, 512);

            memcpy((uint8_t*)(_usb->send_buffer + 8), (uint8_t*)(g_block_buffer + block_index * 32), 32);
            break;
        }

        case CMD_SET_FPGA_DATA_TYPE:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint32_t fpga_data_type = REG32((uint32_t)(_usb->recv_buffer + 4));
            spi0_send_05_via_24(fpga_data_type);

            status = GW_STATUS_SUCCESS;
            break;
        }

        case CMD_GET_SPI0_STATUS:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            status = spi0_recv_0B_via_26();
            break;
        }

        case 0xFACE002A:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint32_t cmd_and_size = REG32((uint32_t)(_usb->recv_buffer + 4));
            uint32_t data = REG32((uint32_t)(_usb->recv_buffer + 8));

            spi0_send_data_24((uint8_t)cmd_and_size, (uint8_t)cmd_and_size >> 8, data);

            status = GW_STATUS_SUCCESS;
            break;
        }

        case 0xFACE002B:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint32_t cmd_and_size = REG32((uint32_t)(_usb->recv_buffer + 4));

            status = spi0_recv_data_26((uint8_t)cmd_and_size, (uint8_t)cmd_and_size >> 8);
            break;
        }

        case 0xFACE0036:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint8_t* data = _usb->recv_buffer + 4;

            // sub_8005E24(data)
            {
                uint32_t tea_key_1 = (data[0] << 16) | (data[1] << 8) | data[2];

                uint32_t tea_key[4] = {
                        tea_key_1,
                        0x95E33D7A,
                        0xDB8EDA54,
                        0x3E21803B
                };

                uint8_t* deciphered_data = data + 3;

                tea_decrypt(tea_key, (uint32_t*)deciphered_data);

                if ( tea_key_1 == 0xFFFFFF )
                {
                    spi0_send_4();
                    //sub_8005BB8();
                    {
                        spi0_transfer_83(0);
                    }

                    spi0_send_8_bytes_via_82();

                    //sub_8005BB2();
                    {
                        spi0_transfer_83(16);
                    }

                    spi0_get_fpga_cmd();
                    spi0_send_11_bytes_0_15_f2_f1_c4(32);
                    spi0_send_11_bytes_0_15_f2_f1_c4(96);
                    spi0_send_11_bytes_0_15_f2_f1_c4(160);
                    spi0_send_11_bytes_0_15_f2_f1_c4(224);
                    spi0_send_4();

                    status = GW_STATUS_SUCCESS;
                    break;
                }
                else
                {
                    if ( !tea_key_1 )
                    {
                        spi0_send_8_bytes_via_82();

                        //sub_8005BB8();
                        {
                            spi0_transfer_83(0);
                        }

                        spi0_get_fpga_cmd();
                    }

                    spi0_write_11_bytes_via_02(deciphered_data);
                    status = spi0_read_status();

                    if ( status == 0xBAD0000F )
                    {
                        uint8_t data_from_03[8];

                        spi0_send_0_C4_via_82();
                        spi0_send_03_read_8_bytes(tea_key_1, data_from_03);

                        spi0_send_8_bytes_via_82();

                        if ( !memcmp(data_from_03, deciphered_data + 3, 8) )
                            status = GW_STATUS_SUCCESS;
                    }
                }
            }

            break;
        }

        case 0xFACE003B:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint8_t* data = _usb->recv_buffer + 4;

            {
                uint32_t tea_key_1 = (data[0] << 16) | (data[1] << 8) | data[2];

                uint32_t tea_key[4] = {
                        tea_key_1,
                        0x95E33D7A,
                        0xDB8EDA54,
                        0x3E21803B
                };

                uint8_t* deciphered_data = data + 3;

                tea_decrypt(tea_key, (uint32_t*)deciphered_data);

                if ( tea_key_1 == 0xFFFFFF )
                {
                    status = GW_STATUS_SUCCESS;
                    break;
                }

                if ( !tea_key_1 )
                {
                    spi0_send_0_C4_via_82();

                    //sub_8005BB8();
                    {
                        spi0_transfer_83(0);
                    }
                }

                uint8_t data_from_03[8];
                spi0_send_03_read_8_bytes(tea_key_1, data_from_03);

                if ( !memcmp(data_from_03, deciphered_data + 3, 8) )
                    status = 0x900D0000;
                else
                    status = 0xBAD00013;
            }

            status = GW_STATUS_SUCCESS;
            break;
        }

        case 0xFACE003C:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            // sub_8005F40
            {
                //sub_8005BB8();
                {
                    spi0_transfer_83(0);
                }

                spi0_send_8_bytes_via_82();

                //sub_8005BB2();
                {
                    spi0_transfer_83(16);
                }

                spi0_get_fpga_cmd();

                spi0_send_11_bytes_30_0_0_1_0(32);
                spi0_send_11_bytes_30_0_0_1_0(96);
                spi0_send_11_bytes_30_0_0_1_0(160);
                spi0_send_11_bytes_30_0_0_1_0(224);

                spi0_send_4();

                uint8_t data_from_03[8];

                // sub_8005E08 (data_from_03)
                {
                    spi0_send_0_C4_via_82();

                    //sub_8005BB2();
                    {
                        spi0_transfer_83(16);
                    }

                    spi0_send_03_read_8_bytes(32, data_from_03);
                    status = GW_STATUS_SUCCESS;
                }

                status = (data_from_03[0] & 48) != 48 ? 0xBAD00012 : GW_STATUS_SUCCESS;
            }

            break;
        }

        case 0xFACE003D:
        {
            if (_is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint32_t spi_psc = REG32((uint32_t)(_usb->recv_buffer + 4));

            status = spi_psc ? spi0_setup(spi_psc) : 0xBAD00002;
            break;
        }

        case 0xFACE003E:
        {
            setup_chip_model_pins();
            status = get_pin_by_board_type();
            break;
        }

        case 0xFACE003F:
        {
            // spi1_setup_and_send_57
            {
                initialize_spi1();

                uint8_t buffer = 0x57;
                spi1_send_data(32, &buffer, 1);
            }

            uint8_t data_from_8F = 0;
            uint16_t data_from_E8[3];

            // uint32_t spi1_recv_8F
            {
                uint8_t buffer = 0;
                spi1_recv_data(0x8F, &buffer, 1);
                data_from_8F = buffer;
            }

            // spi1_read_array_E8
            {
                uint16_t buffer[3];

                spi1_recv_data(0xE8, (uint8_t*)buffer, 6);

                data_from_E8[0] = buffer[0];
                data_from_E8[1] = buffer[1];
                data_from_E8[2] = buffer[2];
            }

            *(uint32_t*)(_usb->send_buffer + 0) = data_from_8F;
            *(uint32_t*)(_usb->send_buffer + 4) = data_from_E8[0];
            *(uint32_t*)(_usb->send_buffer + 8) = data_from_E8[1];
            *(uint32_t*)(_usb->send_buffer + 12) = data_from_E8[2];

            break;
        }

        case 0xFACE0040:
        {
            uint32_t fpga_width = REG32((uint32_t)(_usb->recv_buffer + 4));  // reset fpga settings?

            // sub_8003F4C(fpga_width);
            {
                spi0_send_data_24(0x03, 1, 120u);
                spi0_send_data_24(0x01, 2, 1200u);
                spi0_send_data_24(0x08, 1, 0);
                spi0_send_data_24(0x02, 1, fpga_width);
                spi0_send_data_24(0x09, 1, 0);
                spi0_send_data_24(0x06, 1, 0);
                spi0_send_data_24(0x06, 1, 16u);
                spi0_send_data_24(0x09, 1, 1u);
            }

            for ( int i = 0; i != 256; ++i )
            {
                //if ( gpiof_get_pin6_status() != SET )
                if (gpio_input_bit_get(GPIOF, GPIO_PIN_6) != SET)
                {
                    status = i;
                    break;
                }
            }

            break;
        }

        case 0xFACE0041:
        {
            // sub_8005E08 (_usb->send_buffer)
            {
                spi0_send_0_C4_via_82();

                //sub_8005BB2();
                {
                    spi0_transfer_83(16);
                }

                spi0_send_03_read_8_bytes(32, _usb->send_buffer);
                status = GW_STATUS_SUCCESS;
            }

            break;
        }
    }
}

void handle_firmware_usb_command(bootloader_usb_s* _usb, uint32_t _usb_cmd_size, uint8_t _is_authenticated)
{
    g_usb = _usb;

    if (_usb_cmd_size > 1)
        handle_usb_command(_usb, _usb_cmd_size, _is_authenticated);
    else
        handle_usb_diagnostic(_usb);

    g_usb = 0;
}

uint8_t execute_spi_command(void)
{
    spi0_send_05_recv_BA(1, spi_buffer, sizeof(spi_buffer));
    spi0_send_fpga_cmd(5);

    uint8_t should_send_response = 0;
    uint8_t spi_cmd = spi_buffer[0];

    switch(spi_cmd)
    {
        case 0xA6: // something related to license?
        {
            uint8_t key_from_spi[16];

            memcpy(key_from_spi, (uint8_t*)spi_buffer + 16, sizeof(key_from_spi));

            spi0_send_fpga_cmd(1);
            spi0_send_05_send_BC(3, key_from_spi, 16);
            spi0_send_fpga_cmd(9);

            for(uint32_t i = 18; i > 0; --i)
                spi0_send_05_recv_BA(3, spi_buffer, 512);

            spi0_send_05_recv_BA(3, spi_buffer, 16);

            aes128_cipher(g_aes_keys, key_from_spi);

            memcpy((uint8_t*)spi_buffer + 16, key_from_spi, sizeof(key_from_spi));

            spi0_send_fpga_cmd(1);

            should_send_response = 1;
            break;
        }

        case 0x43:  // request shutdown
        {
            spi_buffer[0] = 0x99;
            should_send_response = 1;
            break;
        }

        case 0x44:  // get bootloader & firmware version and serial
        {
            memset(spi_buffer, 0x11, sizeof(spi_buffer));

            uint32_t* g_bootloader_version = (uint32_t*)((uint32_t)&__bootloader + 0x160);

            spi_buffer[0] = (uint8_t)((*g_bootloader_version) & 0xFF);
            spi_buffer[1] = (uint8_t)((*g_bootloader_version >> 8) & 0xFF);

            uint32_t* g_firmware_version = (uint32_t*)((uint32_t)&__firmware + 0x158);

            spi_buffer[2] = (uint8_t)((*g_firmware_version) & 0xFF);
            spi_buffer[3] = (uint8_t)((*g_firmware_version >> 8) & 0xFF);

            uint8_t* g_serial_number = (uint8_t*)((uint32_t)&__bootloader + 0x150);

            memcpy((uint8_t*)spi_buffer + 4, g_serial_number, 16);

            spi_buffer[24] = spi0_recv_data_26(0x0C, 1);

            should_send_response = 1;
            break;
        }

        case 0x55:  // shutdown
        {
            shutdown();
            break;
        }

        case 0x66: // switch into bootloader spi cmd handler
        {
            ((reset_function_t)__bootloader + 0x164)();
            break;
        }

        case 0x77:
        {
            // set sp to NVIC_NVIC_VECTTAB_RAM, call run_glitch, run handle_firmware_spi_command

            break;
        }

        case 0x88: // reset fpga & shutdown
        {
            spi0_send_fpga_cmd(0x80);
            delay_ms(1);
            spi0_send_fpga_cmd(0);
            shutdown();
            break;
        }

        case 0x98: // erase config page
        {
            // TODO: do page size alignment on g_saved_config inorder to just erase it.
            uint32_t status = flash_erase((uint32_t)&g_saved_config);

            *(uint32_t*)(spi_buffer) = status;
            should_send_response = 1;
            break;
        }

        case 0x99: // enable/disable modchip
        {
            uint32_t status = 0;
            config_s config;

            config_load_from_flash(&config);
            config_set_chip_enabled(&config, spi_buffer[1]);
            status = config_write_to_flash(&config);

            *(uint32_t*)(spi_buffer) = status;
            should_send_response = 1;
            break;
        }

        default:
            break;
    }

    if (should_send_response)
    {
        spi0_send_05_send_BC(1, spi_buffer, 512);
        spi0_send_fpga_cmd(3);
    }

    return spi_cmd;
}

void handle_firmware_spi_command(void)
{
    while(1)
    {
        while ( (spi0_recv_0B_via_26() & 0x10) == 0 );

        execute_spi_command();
    }
}

int main(void)
{
    hardware_initialize();

    setup_adc_for_gpio_pin(GPIOA, GPIO_PIN_3, ADC_CHANNEL_3);
    uint32_t adc_value = adc_channel_read();

    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_9);
    gpio_input_bit_get(GPIOA, GPIO_PIN_9);

    if ( !is_chip_disabled() )
    {
        if (spi0_init_with_psc_4() != GW_STATUS_SUCCESS)
        {
            set_led_color(LED_COLOR_RED);
            handle_firmware_spi_command();
        }

        if (adc_value < 1496)
        {
            run_glitch(0, 0);
            handle_firmware_spi_command();
        }

        //
        {
            timeout_s timeout;

            uint8_t* g_serial_number = (uint8_t*)((uint32_t)&__bootloader + 0x150);
            spi0_send_05_send_BC(6, (uint8_t*)g_serial_number, 16);

            spi0_send_07_via_24(1); // status set?
            spi0_send_07_via_24(0); // status clear?

            timeout_initialize(&timeout);

            do
            {
                if ( (spi0_recv_0B_via_26() & 0x80) != 0 )
                {
                    spi0_send_fpga_cmd(4);
                    spi0_send_fpga_cmd(1);
                    handle_firmware_spi_command();
                }

                timeout_update(&timeout);
            }
            while ( !timeout_did_reach_timeout_ms(&timeout, 5000) );
        }
    }

    shutdown();

    return 0;
}