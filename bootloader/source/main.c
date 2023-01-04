#include "gd32f3x0_it.h"
#include "gd32f3x0_libopt.h"
#include "gw_defines.h"

#include "usb.h"
#include "spi.h"
#include "delay.h"
#include "rgb_led.h"
#include "firmware_update.h"
#include "initialization.h"

extern uint8_t* __spi_buffer__;

void initialize_vector_table()
{
	nvic_vector_table_set((uint32_t)&__firmware, 0);

	// call reset_handler inside firmware
	((void_function_t)((uint32_t)&__firmware) + 4)();
}

void handle_bootloader_spi_commands(void)
{
	spi0_send_fpga_cmd(1);

	while ( TRUE )
	{
		while ( TRUE )
		{
			// NOTE: polling for data?
			while ((spi0_recv_0B_via_26() & 0x10) == 0);

			spi0_send_05_recv_BA(1, __spi_buffer__, 512);

			spi0_send_fpga_cmd(5);

			uint32_t status_code = 0;

			uint8_t spi_cmd =  __spi_buffer__[0];

			switch ( spi_cmd )
			{
				case MC_FW_UPDATE_INIT:
				{
					status_code = validate_firmware_header(__spi_buffer__ + 16, __spi_buffer__[1]);
					break;
				}

				case MC_FW_UPDATE_TRANSFER:
				{
					for(uint32_t i = 0; i != 0x1D0; i += 64)
					{
						status_code = handle_firmware_update((__spi_buffer__ + 16) + i);

						if (status_code != GW_STATUS_RECEIVED_UPDATE_BLOCK)
							break;
					}

					break;
				}

				case MC_SET_LED_COLOR:
				{
					uint32_t color = REG32(__spi_buffer__ + 16);
					set_led_color(color);
					status_code = GW_STATUS_SUCCESS;
					break;
				}

				case MC_ENTER_DEEPSLEEP_BLDR:
				{
					shutdown();
					break;
				}

				case MC_SWITCH_TO_FW:
				{
					uint32_t* flash_function_table = (uint32_t*)FW_FUNC_TBL;
					uint32_t* did_initialize_functions = (uint32_t*)FW_IS_INITIALIZED;

					if (*debug_mode != BOOTLOADER_DEBUG_MODE && *did_initialize_functions)
						break;

					g_ram_function_table = flash_function_table;
					status_code = GW_STATUS_RESET;
					break;
				}

				default:
					// how did we end up here?
					break;
			}

			REG32(__spi_buffer__) = status_code;
			spi0_send_05_send_BC(1, __spi_buffer__, 512);
			spi0_send_fpga_cmd(3);

			// NOTE: not a firmware update command, go back into the first while loop
			if (spi_cmd != MC_FW_UPDATE_INIT && spi_cmd != MC_FW_UPDATE_TRANSFER)
				break;
		}
	}
}

int main(void)
{
	rcu_periph_clock_enable(RCU_GPIOA);

	gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_9);
	FlagStatus usb_vbus = gpio_input_bit_get(GPIOA, GPIO_PIN_9);

	uint32_t* did_initialize_functions = (uint32_t*)FW_IS_INITIALIZED;

	// check if usb vbus is present
	if ((*debug_mode == BOOTLOADER_DEBUG_MODE || !*did_initialize_functions) && usb_vbus == RESET)
		initialize_vector_table();

	hardware_initialize();

	set_led_color(LED_COLOR_GREEN);

	handle_usb_transfers();
	return 0;
}