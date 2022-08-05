#include "gd32f3x0_libopt.h"
#include "gd32f3x0_it.h"

#include "initialization.h"
#include "delay.h"
#include "spi.h"
#include "rgb_led.h"

void initialize_usart(void)
{
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

void initialize_timers()
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

	//timer_parameter.prescaler = rcu_clock_freq_get(CK_SYS) / 1000000;
	timer_parameter.prescaler = 107; // kHz - 1 / 1000000 or MHz - 1?
	timer_parameter.alignedmode = TIMER_COUNTER_EDGE;
	timer_parameter.counterdirection = TIMER_COUNTER_UP;
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

#ifdef FIRMWARE
	// setup_timer13_irq
	nvic_irq_enable(TIMER13_IRQn, 1, 1);
#endif
}

void initialize_ckout(void)
{
	rcu_ckout_config(RCU_CKOUTSRC_CKPLL_DIV2, 1);

	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_8);
	gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_8);
}

void initialize_spi_0(void)
{
	initialize_spi0(SPI_PSC_2);

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

	gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_10);	// iCE40 CRESET
	gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_1); 	// iCE40 CDONE
	gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_4);	// SPI0 NSS
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_5);		// SPI0 SCK
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_6);		// SPI0 MISO
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_7);		// SPI0 MOSI
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
		uint32_t usbfs_prescaler = RCU_USBFS_CKPLL_DIV2;

		// NOTE: replaced "usbfs_prescaler = RCU_USBFS_CKPLL_DIV2" with proper code below for support of dynamic clocks
		/*switch(rcu_clock_freq_get(CK_SYS))
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
		}*/

		rcu_usbfs_clock_config(usbfs_prescaler);

		rcu_periph_clock_enable(RCU_USBFS);

		rcu_periph_clock_enable(RCU_PMU);
	}

	rcu_periph_clock_enable(RCU_USART0);

#ifdef FIRMWARE
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_GPIOF);

	rcu_periph_clock_enable(RCU_SPI0);
#else
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOB);
#endif

	rcu_periph_clock_enable(RCU_TIMER0);
	rcu_periph_clock_enable(RCU_TIMER13);
	rcu_periph_clock_enable(RCU_TIMER14);
	rcu_periph_clock_enable(RCU_TIMER15);
	rcu_periph_clock_enable(RCU_TIMER16);

#ifdef FIRMWARE
	initialize_usart();

	initialize_timers();

	initialize_ckout();

	initialize_spi_0();

	setup_chip_model_pins();

	// setup_gpiof_pin6
	{
		gpio_mode_set(GPIOF, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_6);
	}
#else
	initialize_timers();
#endif

	delay_init();
}

void shutdown()
{
	set_led_color(LED_COLOR_OFF);
	fpga_spi0_set_creset();

	while ( TRUE )
		pmu_to_standbymode(0);
}