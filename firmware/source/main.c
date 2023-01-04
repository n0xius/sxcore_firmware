#include "gd32f3x0_it.h"
#include "gd32f3x0_libopt.h"

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
#include "initialization.h"

extern void retry();

uint8_t g_block_buffer[512];
uint8_t spi_buffer[512];

bootloader_usb_s* g_usb = 0;

const uint8_t g_aes_keys[16] = {
	0x7B, 0xC7, 0x6D, 0x76,
	0xFF, 0x45, 0xEA, 0xFA,
	0x0C, 0x3B, 0x73, 0x0A,
	0xCC, 0x25, 0x93, 0x00
};

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

void set_start_addr_to_firmware()
{
	nvic_vector_table_set((uint32_t)__firmware, 0);
	hardware_initialize();
}

void handle_usb_command(bootloader_usb_s* _usb, uint32_t _usb_cmd_size, uint8_t _is_authenticated)
{
	if (_usb_cmd_size <= 3)
		return;

	uint32_t cmd = REG32(_usb->recv_buffer);
	uint32_t status = 0;

	switch(cmd)
	{
		case USB_CMD_PING:
		{
			status = GW_STATUS_SUCCESS;
			REG32(_usb->send_buffer) = status;
			break;
		}

		case USB_CMD_SPI0_INITALIZATION:
		{
			status = spi0_init_with_psc_4();

			REG32(_usb->send_buffer) = status;
			break;
		}

		case USB_CMD_RESET_FPGA:
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			spi0_send_fpga_cmd(0x80);
			delay_ms(100);
			spi0_send_fpga_cmd(0);

			if ( REG32(_usb->recv_buffer + 4) == 0xAABBCCDD )
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

		case USB_CMD_SEND_FPGA_CMD:
		case USB_CMD_SEND_FPGA_CMD_1:
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			uint32_t fpga_cmd = REG32(_usb->recv_buffer + 4);

			spi0_send_fpga_cmd(fpga_cmd);

			status = GW_STATUS_SUCCESS;
			break;
		}

		case USB_CMD_MMC_SEND_CMD:
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			uint32_t mmc_cmd = REG32(_usb->recv_buffer + 4);
			uint32_t mmc_arg = REG32(_usb->recv_buffer + 8);
			mmc_send_command(mmc_cmd, mmc_arg, _usb->send_buffer, 0);
			break;
		}

		case USB_CMD_MMC_WRITE_BLOCK:
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			uint32_t block_index = REG32(_usb->recv_buffer + 4) & 15;

			memcpy((uint8_t*)(g_block_buffer + block_index * 32), (_usb->recv_buffer + 8), 32);

			if ( block_index == 15 )
				spi0_send_05_send_BC(1, g_block_buffer, 512);

			status = GW_STATUS_SUCCESS;
			break;
		}

		case USB_CMD_MMC_READ_BLOCK:
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			uint32_t block_index = REG32(_usb->recv_buffer + 4) & 15;

			if ( block_index == 0 )
				spi0_send_05_send_BC(1, g_block_buffer, 512);

			memcpy((_usb->send_buffer + 8), (uint8_t*)(g_block_buffer + block_index * 32), 32);
			break;
		}

		case USB_CMD_MMC_COPY_BLOCK:
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			uint32_t block_index = REG32(_usb->recv_buffer + 4) & 15;

			if ( block_index == 0 )
				spi0_recv_data_BA(g_block_buffer, 512);

			memcpy((_usb->send_buffer + 8), (uint8_t*)(g_block_buffer + (block_index * 32)), 32);
			break;
		}

		case USB_CMD_FPGA_SET_CMD_BUFFER: // fpga set active data buffer
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			uint32_t fpga_data_type = REG32(_usb->recv_buffer + 4);
			spi0_send_05_via_24(fpga_data_type);

			status = GW_STATUS_SUCCESS;
			break;
		}

		case USB_CMD_FPGA_GET_STATUS_FLAGS: // fpga read mmc flags?
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			status = spi0_recv_0B_via_26();
			break;
		}

		case USB_CMD_FPGA_SET_GLITCH_PARAM: // fpga set glitch parameter?
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			uint32_t cmd_and_size = REG32(_usb->recv_buffer + 4);
			uint32_t data = REG32(_usb->recv_buffer + 8);

			spi0_send_data_24(cmd_and_size, data);

			status = GW_STATUS_SUCCESS;
			break;
		}

		case USB_CMD_FPGA_GET_GLITCH_PARAM: // fpga get glitch parameter?
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			uint32_t cmd_and_size = REG32(_usb->recv_buffer + 4);
			
			status = spi0_recv_data_26(cmd_and_size);
			break;
		}

		case 0xFACE0036: // fpga write and compare nvcm data
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			uint8_t* data = _usb->recv_buffer + 4;

			// sub_8005E24(data)
			{
				uint32_t fpga_address = (data[0] << 16) | (data[1] << 8) | data[2];

				uint32_t tea_key[4] = {
						fpga_address,
						0x95E33D7A,
						0xDB8EDA54,
						0x3E21803B
				};

				uint8_t* deciphered_data = data + 3;

				tea_decrypt(tea_key, (uint32_t*)deciphered_data);

				if ( fpga_address == 0xFFFFFF )
				{
					spi0_send_4();

					// fpga bank select: nvcm (0x00)
					{
						fpga_select_bank(0);
					}

					spi0_send_8_bytes_via_82();

					// fpga bank select: trim (0x10)
					{
						fpga_select_bank(0x10);
					}

					spi0_get_fpga_cmd();
					spi0_send_11_bytes_0_15_f2_f1_c4(0x20); // write trim0
					spi0_send_11_bytes_0_15_f2_f1_c4(0x60); // write trim1
					spi0_send_11_bytes_0_15_f2_f1_c4(0xA0); // write trim2
					spi0_send_11_bytes_0_15_f2_f1_c4(0xE0); // write trim3
					spi0_send_4(); // disable write

					status = GW_STATUS_SUCCESS;
					break;
				}
				else
				{
					if ( !fpga_address )
					{
						spi0_send_8_bytes_via_82();

						// fpga bank select: nvcm (0x00)
						{
							fpga_select_bank(0);
						}

						spi0_get_fpga_cmd();
					}

					spi0_write_11_bytes_via_02(deciphered_data); // write trim
					status = spi0_read_status();

					if ( status == 0xBAD0000F )
					{
						uint8_t data_from_03[8];

						spi0_send_0_C4_via_82();
						fpga_nvcm_read(fpga_address, data_from_03);

						spi0_send_8_bytes_via_82();

						if ( !memcmp(data_from_03, deciphered_data + 3, 8) )
							status = GW_STATUS_SUCCESS;
					}
				}
			}

			break;
		}

		case 0xFACE003B: // fpga read and compare nvcm data
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			uint8_t* data = _usb->recv_buffer + 4;

			{
				uint32_t fpga_address = (data[0] << 16) | (data[1] << 8) | data[2];

				uint32_t tea_key[4] = {
						fpga_address,
						0x95E33D7A,
						0xDB8EDA54,
						0x3E21803B
				};

				uint8_t* deciphered_data = data + 3;

				tea_decrypt(tea_key, (uint32_t*)deciphered_data);

				if ( fpga_address == 0xFFFFFF )
				{
					status = GW_STATUS_SUCCESS;
					break;
				}

				if ( !fpga_address )
				{
					spi0_send_0_C4_via_82();

					// fpga bank select: nvcm (0x00)
					{
						fpga_select_bank(0);
					}
				}

				uint8_t data_from_03[8];
				fpga_nvcm_read(fpga_address, data_from_03);

				if ( memcmp(data_from_03, deciphered_data + 3, 8) != 0 )
					status = GW_STATUS_FPGA_NVCM_READ_MISMATCH; // 0xBAD00013
			}

			status = GW_STATUS_SUCCESS;
			break;
		}

		case 0xFACE003C: // fpga secure and verify trim
		{
			if (_is_authenticated == AUTH_NO_KEY)
			{
				status = GW_STATUS_AUTH_REQUIRED;
				break;
			}

			// sub_8005F40
			{
				// fpga bank select: nvcm (0x00)
				{
					fpga_select_bank(0);
				}

				spi0_send_8_bytes_via_82();

				// fpga bank select: trim (0x10)
				{
					fpga_select_bank(0x10);
				}

				spi0_get_fpga_cmd();

				spi0_send_11_bytes_30_0_0_1_0(0x20);
				spi0_send_11_bytes_30_0_0_1_0(0x60);
				spi0_send_11_bytes_30_0_0_1_0(0xA0);
				spi0_send_11_bytes_30_0_0_1_0(0xE0);

				spi0_send_4();

				uint8_t data_from_03[8];

				// sub_8005E08 (data_from_03)
				{
					spi0_send_0_C4_via_82();

					// fpga bank select: trim (0x10)
					{
						fpga_select_bank(0x10);
					}

					// get nvcm Trim_Parameter_OTP
					fpga_nvcm_read(0x20, data_from_03);
					status = GW_STATUS_SUCCESS;
				}

				status = (data_from_03[0] & 0x30) != 0x30 ? GW_STATUS_FPGA_TRIM_SECURE_FAILED : GW_STATUS_SUCCESS;
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

			uint32_t spi_psc = REG32(_usb->recv_buffer + 4);

			status = spi_psc ? spi0_setup(spi_psc) : GW_STATUS_RESET;
			break;
		}

		case USB_CMD_GET_BOARD_ID: // get board type?
		{
			setup_chip_model_pins();
			status = get_pin_by_board_type();
			break;
		}

		case USB_CMD_GET_DATA_FROM_LIS3DH:
		{
			// spi1_setup_and_send_57
			{
				initialize_spi1();

				uint8_t buffer = 0b01010111; // HR / Normal / Low-power mode (100 Hz), X-enable, Y-enable, Z-enable
				spi1_send_data(0x20, &buffer, 1); // CTRL_REG1
			}

			uint8_t data_from_8F = 0;
			uint16_t data_from_E8[3];

			// uint32_t spi1_recv_8F
			{
				uint8_t buffer = 0;
				spi1_recv_data(0x8F, &buffer, 1); // WHO_AM_I
				data_from_8F = buffer;
			}

			// spi1_read_array_E8
			{
				uint16_t buffer[3];

				spi1_recv_data(0xE8, (uint8_t*)buffer, 6);

				data_from_E8[0] = buffer[0]; // read x axis
				data_from_E8[1] = buffer[1]; // read y axis
				data_from_E8[2] = buffer[2]; // read z axis
			}

			REG32(_usb->send_buffer + 0) = data_from_8F;
			REG32(_usb->send_buffer + 4) = data_from_E8[0];
			REG32(_usb->send_buffer + 8) = data_from_E8[1];
			REG32(_usb->send_buffer + 12) = data_from_E8[2];

			break;
		}

		case USB_CMD_RESET_FPGA_GLITCH_CFG:
		{
			uint32_t fpga_width = REG32(_usb->recv_buffer + 4);

			// fpga_reset_glitch_settings(fpga_width);
			{
				spi0_send_data_24(0x103, 120);       	// timeout?
				spi0_send_data_24(0x201, 1200);      	// offset?
				spi0_send_data_24(0x108, 0);          	// subcycle delay?
				spi0_send_data_24(0x102, fpga_width); 	// pulse width
				spi0_send_data_24(0x109, 0);
				spi0_send_data_24(0x106, 0);
				spi0_send_data_24(0x106, 16);
				spi0_send_data_24(0x109, 1);
			}

			for ( int i = 0; i != 256; ++i )
			{
				if (gpio_input_bit_get(GPIOF, GPIO_PIN_6) != SET)
				{
					status = i;
					break;
				}
			}

			break;
		}

		case 0xFACE0041: // fpga get Trim_Parameter_OTP
		{
			// fpga_get_trim_parameter_otp
			{
				spi0_send_0_C4_via_82();

				// fpga bank select: trim (0x10)
				{
					fpga_select_bank(0x10);
				}

				fpga_nvcm_read(0x20, _usb->send_buffer);
				status = GW_STATUS_SUCCESS;
			}

			break;
		}
	}
}

void handle_firmware_usb_commands(bootloader_usb_s* _usb, uint32_t _usb_cmd_size, uint8_t _is_authenticated)
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
		case MC_REQUEST_SHUTDOWN:  // request shutdown
		{
			spi_buffer[0] = 0x99;
			should_send_response = 1;
			break;
		}

		case MC_GET_SERIAL_INFO:  // get bootloader & firmware version and serial
		{
			memset(spi_buffer, 0x11, sizeof(spi_buffer));

			uint8_t* serial_number = (uint8_t*)BLDR_SERIAL;
			uint32_t* bootloader_version = (uint32_t*)BLDR_VERSION;

			spi_buffer[0] = (uint8_t)((*bootloader_version) & 0xFF);
			spi_buffer[1] = (uint8_t)((*bootloader_version >> 8) & 0xFF);

			spi_buffer[2] = (uint8_t)((*firmware_version) & 0xFF);
			spi_buffer[3] = (uint8_t)((*firmware_version >> 8) & 0xFF);

			memcpy((uint8_t*)spi_buffer + 4, serial_number, 16);

			spi_buffer[24] = spi0_recv_data_26(0x10C);

			should_send_response = 1;
			break;
		}

		case MC_ENTER_DEEPSLEEP_FW:  // shutdown
		{
			shutdown();
			break;
		}

		case MC_SWITCH_TO_BLDR: // switch into bootloader spi cmd handler
		{
			// call handle_spi_cmds inside the bootloader
			((irq_handler_t)BLDR_SPI_HANDLER)();
			break;
		}

		case MC_REBOOT_DEVICE:
		{
			// set sp to NVIC_NVIC_VECTTAB_RAM, call run_glitch, run handle_firmware_spi_command
			retry();
			break;
		}

		case MC_BOOT_OFW: // reset fpga & shutdown
		{
			spi0_send_fpga_cmd(0x80);
			delay_ms(1);
			spi0_send_fpga_cmd(0);
			shutdown();
			break;
		}

		case MC_ERASE_CFG: // erase config page
		{
			uint32_t status = flash_erase((uint32_t)__config);

			REG32(spi_buffer) = status;
			should_send_response = 1;
			break;
		}

		case MC_TOGGLE_CHIP: // enable/disable modchip
		{
			uint32_t status = 0;
			config_s config;

			config_load_from_flash(&config);
			config_set_chip_enabled(&config, spi_buffer[1]);
			status = config_write_to_flash(&config);

			REG32(spi_buffer) = status;
			should_send_response = 1;
			break;
		}
		
		case MC_LICENSE_RNG: // something related to license?
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

void handle_firmware_spi_commands(void)
{
	while ( TRUE )
	{
		while ( (spi0_recv_0B_via_26() & 0x10) == 0 );

		execute_spi_command();
	}
}

void listen_for_spi_commands()
{
	uint8_t* serial_number = (uint8_t*)BLDR_SERIAL;
	spi0_send_05_send_BC(6, serial_number, 16);

	spi0_send_07_via_24(1); // status set?
	spi0_send_07_via_24(0); // status clear?

	timeout_s timeout;
	timeout_initialize(&timeout);

	do
	{
		if ( (spi0_recv_0B_via_26() & 0x80) != 0 )
		{
			// fpga enter cmd mode
			spi0_send_fpga_cmd(4);
			spi0_send_fpga_cmd(1);

			handle_firmware_spi_commands();
		}

		timeout_update(&timeout);
	}
	while ( !timeout_did_reach_timeout_ms(&timeout, 5000) );
}

int main(void)
{
	hardware_initialize();

	setup_adc_for_gpio_pin(GPIOA, GPIO_PIN_3, ADC_CHANNEL_3);
	uint16_t adc_value = adc_channel_read();

	// NOTE: listen for usb vbus via PA09
	gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO_PIN_9);
	gpio_input_bit_get(GPIOA, GPIO_PIN_9);

	if ( !is_chip_disabled() )
	{
		if (spi0_init_with_psc_4() != GW_STATUS_SUCCESS)
		{
			set_led_color(LED_COLOR_RED);
			handle_firmware_spi_commands();
		}

		if (adc_value < 1496)
		{
			run_glitch(&g_no_diagnosis_print, 0);
			handle_firmware_spi_commands();
		}

		listen_for_spi_commands();
	}

	shutdown();

	return 0;
}