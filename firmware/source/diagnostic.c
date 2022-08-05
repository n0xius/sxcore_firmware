#include "gd32f3x0_it.h"
#include "gw_defines.h"

#include "diagnostic.h"
#include "configuration.h"
#include "glitch.h"
#include "flash.h"
#include "emmc.h"
#include "spi.h"

extern bootloader_usb_s* g_usb;

extern uint32_t get_pin_by_board_type();

void diagnosis_print_begin();
void diagnosis_print_hexdump(uint8_t _type, uint8_t *_data, uint32_t _size);
void diagnosis_print_end();

const diagnostic_print_s g_diagnosis_print = {
		&diagnosis_print_begin,
		&diagnosis_print_hexdump,
		&diagnosis_print_end
};

const diagnostic_print_s g_no_diagnosis_print = {
		0,
		0,
		0
};

uint32_t diagnose_report_len;
uint32_t diagnosis_cur_xor_key;

void diagnosis_printf(const char* _text, ...)
{
	if (!g_usb)
		return;

	va_list list;
	va_start(list, _text);

	int length = _vsnprintf((char*)g_usb->send_buffer, (uint32_t)64, _text, list);

	if (length)
		g_usb->send_func(length);
}

uint8_t diagnosis_xor_key(void)
{
	uint32_t v0 = diagnosis_cur_xor_key;

	for(int i = 0; i < 8; ++i)
		v0 = (((int)(v0 << 31) >> 31) & 0xA3000000) ^ (v0 >> 1);

	diagnosis_cur_xor_key = v0;
	return (uint8_t)v0;
}

void diagnosis_print_begin()
{
	diagnose_report_len = 0;
	diagnosis_cur_xor_key = 0xF851AB29;
	diagnosis_printf("# Diagnose report:\n");
}

void diagnosis_print_hexdump(uint8_t _type, uint8_t *_data, uint32_t _size)
{
	for ( uint32_t i = 0; _size + 1 > i; ++i )
	{
		uint8_t data = i ? _data[i - 1] : _type;

		diagnosis_printf("%02X ", diagnosis_xor_key() ^ data);

		if ( (uint8_t)(diagnose_report_len + 1) > 31u )
		{
			diagnose_report_len = 0;
			diagnosis_printf("\n");
		}
		else
		{
			++diagnose_report_len;
		}
	}
}

void diagnosis_print_end()
{
	if ( diagnose_report_len )
		diagnosis_printf("\n");
}

void handle_usb_diagnostic(bootloader_usb_s* _usb)
{
	switch(_usb->recv_buffer[0])
	{
		case UDC_BOOT_OFW:
		{
			diagnosis_printf("> b\n");
			diagnosis_printf("# Boot\n");

			uint32_t status = spi0_init_with_psc_4();

			if ( status == GW_STATUS_SUCCESS )
				status = glitch_and_boot();

			diagnosis_printf("# Status: %08X\n", status);

			if ( status == GW_STATUS_SUCCESS )
				diagnosis_printf("# Success!\n");

			break;
		}

		case UDC_RUN_DIAGNOSIS:
		{
			diagnosis_printf("> d\n");
			diagnosis_printf("# Diagnosing...\n");

			uint32_t status = spi0_init_with_psc_4();

			if ( status == GW_STATUS_SUCCESS )
				status = run_glitch(&g_diagnosis_print, 0);

			diagnosis_printf("# Diagnose status: %08X\n", status);

			if ( status == GW_STATUS_NO_OR_UNKNOWN_DEVICE )
			{
				diagnosis_printf("# Please make sure console is powered on\n");
				return;
			}

			if ( status == GW_STATUS_GLITCH_SUCCESS )
				diagnosis_printf("# Success!\n");

			break;
		}

		case UDC_EMMC_PAYLOAD_ERASE:
		{
			diagnosis_printf("> e\n");
			diagnosis_printf("# Erasing eMMC payload...\n");

			uint32_t device = DEVICE_TYPE_UNKNOWN;
			uint32_t status = spi0_init_with_psc_4();

			if ( status == GW_STATUS_SUCCESS )
			{
				status = glitch_and_get_device_type(&device);

				if ( status == GW_STATUS_SUCCESS )
					status = reset_bct_and_erase_payload();
			}

			diagnosis_printf("# Status: %08X\n", status);

			if ( status == GW_STATUS_NO_OR_UNKNOWN_DEVICE ) // 0xBAD00107
				diagnosis_printf("# Please make sure console is powered on\n");

			break;
		}

		case UDC_EMMC_PAYLOAD_PROGRAM:
		{
			diagnosis_printf("> p\n");
			diagnosis_printf("# Programming eMMC payload...\n");
			uint32_t status = spi0_init_with_psc_4();
			uint8_t cid_data[16];

			if ( status == GW_STATUS_SUCCESS )
			{
				uint32_t device;
				status = glitch_and_get_device_type(&device);

				if ( status == GW_STATUS_SUCCESS )
					status = write_bct_and_payload(cid_data, device);
			}

			if ( status == GW_STATUS_BCT_PAYLOAD_SUCCESS )
			{
				diagnosis_printf("# CID: ");

				for ( uint32_t i = 0; i < 16; ++i )
					diagnosis_printf("%02X", cid_data[i]);

				diagnosis_printf(" ");

				switch ( cid_data[0] )
				{
					case 0x15:
					{
						diagnosis_printf("Samsung");

						if ( !memcmp((uint8_t *)cid_data + 3, (uint8_t *)"BJTD4R", (uint32_t)6) )
							diagnosis_printf(" KLMBG2JETD-B041 32GB");

						if ( !memcmp((uint8_t *)cid_data + 3, (uint8_t *)"BJNB4R", (uint32_t)6) )
							diagnosis_printf(" KLMBG2JENB-B041 32GB");

						break;
					}

					case 0x11:
					{
						diagnosis_printf("Toshiba");

						if ( !memcmp((uint8_t *)cid_data + 3, (uint8_t *)"032G32", (uint32_t)6) )
							diagnosis_printf(" THGBMHG8C2LBAIL 32GB");

						break;
					}

					case 0x90:
					{
						diagnosis_printf("Hynix");

						if ( !memcmp((uint8_t *)cid_data + 3, (uint8_t *)"hB8aP>", (uint32_t)6) )
							diagnosis_printf(" H26M62002JPR 32GB");

						break;
					}
				}

				diagnosis_printf("\n");
			}

			diagnosis_printf("# Status: %08X\n", status);

			if ( status == GW_STATUS_NO_OR_UNKNOWN_DEVICE ) // 0xBAD00107
				diagnosis_printf("# Please make sure console is powered on\n");

			break;
		}

		case UDC_FACTORY_RESET:
		{
			diagnosis_printf("> r\n");
			diagnosis_printf("# Resetting to factory settings...\n");

			uint32_t status = flash_erase((uint32_t)__config);

			diagnosis_printf("# Status: %08X\n", status);
			break;
		}

		case UDC_TOGGLE_CHIP:
		{
			diagnosis_printf("> s\n");

			config_s cfg;
			config_load_from_flash(&cfg);

			uint8_t chip_enabled = config_is_chip_enabled(&cfg);

			diagnosis_printf("# Setting mode to: ");

			if ( chip_enabled )
				diagnosis_printf("enabled\n");
			else
				diagnosis_printf("disabled\n");

			config_set_chip_enabled(&cfg, !chip_enabled);
			uint32_t status = config_write_to_flash(&cfg);

			diagnosis_printf("# Status: %08X\n", status);

			if ( status == GW_STATUS_CONFIG_SUCCESS )
				diagnosis_printf("# Success!\n");

			break;
		}

		case UDC_RUN_TEST:
		{
			diagnosis_printf("> t\n");
			diagnosis_printf("# Testing...\n");

			uint32_t status = spi0_init_with_psc_4();

			if ( status == GW_STATUS_SUCCESS )
				status = run_glitch(&g_diagnosis_print, &g_fpga_config);

			diagnosis_printf("# Testing status: %08X\n", status);

			if ( status == GW_STATUS_NO_OR_UNKNOWN_DEVICE ) // 0xBAD00107
				diagnosis_printf("# Please make sure console is powered on\n");

			if ( status == GW_STATUS_GLITCH_SUCCESS )
				diagnosis_printf("# Success!\n");

			break;
		}

		case UDC_GET_CHIP_INFO:
		{
			diagnosis_printf("> v\n");
			diagnosis_printf("# SX FW v%d.%d\n", (uint8_t)(*firmware_version >> 8), (uint8_t)(*firmware_version));

			uint32_t pin = get_pin_by_board_type();

			diagnosis_printf("# Board ID: ");

			if ( pin == GPIO_PIN_0 )
				diagnosis_printf("SX Core\n");
			else if ( pin == GPIO_PIN_1 )
				diagnosis_printf("SX Lite\n");
			else
				diagnosis_printf("Unknown\n");

			break;
		}

		case UDC_FPGA_WIDTH_INCREASE:
		case UDC_FPGA_WIDTH_DECREASE:
		case UDC_FPGA_RNG_INCREASE:
		case UDC_FPGA_RNG_DECREASE:
		case UDC_FPGA_OFFSET_INCREASE:
		case UDC_FPGA_OFFSET_DECREASE:
		{
			if (_usb->recv_buffer[0] == UDC_FPGA_WIDTH_INCREASE)
				g_fpga_config.width += 1;

			if (_usb->recv_buffer[0] == UDC_FPGA_WIDTH_DECREASE)
				g_fpga_config.width -= 1;

			if (_usb->recv_buffer[0] == UDC_FPGA_RNG_INCREASE)
				g_fpga_config.rng += 1;

			if (_usb->recv_buffer[0] == UDC_FPGA_RNG_DECREASE)
				g_fpga_config.rng -= 1;

			if (_usb->recv_buffer[0] == UDC_FPGA_OFFSET_INCREASE)
				g_fpga_config.offset += 1;

			if (_usb->recv_buffer[0] == UDC_FPGA_OFFSET_DECREASE)
				g_fpga_config.offset -= 1;

			diagnosis_printf("# %d:%d %d\n", g_fpga_config.offset, g_fpga_config.rng, g_fpga_config.width);
			break;
		}

		default:
			break;
	}
}

void diagnosis_begin(const diagnostic_print_s *_diag_print)
{
	if (!_diag_print)
		return;

	_diag_print->begin();
}

void diagnosis_hexdump(const diagnostic_print_s *_diag_print, uint8_t _type, uint8_t *_data, uint32_t _size)
{
	if (!_diag_print)
		return;

	_diag_print->hexdump(_type, _data, _size);
}

void diagnosis_end(const diagnostic_print_s *_diag_print)
{
	if (!_diag_print)
		return;

	_diag_print->end();
}

void diagnosis_hexdump_spi_start(const diagnostic_print_s *_diag_print)
{
	diagnosis_hexdump(_diag_print, DATA_TYPE_FPGA_START, 0, 0);
}

void diagnosis_hexdump_fpga(const diagnostic_print_s *_diag_print, fpga_config_s *_fpga_cfg, uint8_t _spi_status, uint32_t _data_size, uint8_t *_data, uint8_t a6, uint8_t a7)
{
	if ( _data_size >= 48 )
		_data_size = 48;

	uint8_t data_header[8];

	data_header[0] = _spi_status;
	data_header[1] = _fpga_cfg->width;
	data_header[2] = (uint8_t)(_fpga_cfg->offset);
	data_header[3] = (uint8_t)(_fpga_cfg->offset >> 8);
	data_header[4] = a6;
	data_header[5] = _data_size;
	data_header[6] = a7;
	data_header[7] = _fpga_cfg->rng;

	diagnosis_hexdump(_diag_print, DATA_TYPE_FPGA_HEADER, data_header, 8);
	diagnosis_hexdump(_diag_print, DATA_TYPE_FPGA, _data, _data_size);
}

void diagnosis_hexdump_config(const diagnostic_print_s *_diag_print, config_s *_cfg)
{
	diagnosis_hexdump(_diag_print, DATA_TYPE_CONFIG, (uint8_t *)_cfg, sizeof(config_s));
}

void diagnosis_hexdump_fpga_cfg(const diagnostic_print_s *_diag_print, fpga_config_s *_fpga_cfg, uint32_t _status)
{
	uint32_t data[4];

	data[0] = _fpga_cfg->width;
	data[1] = _fpga_cfg->offset;
	data[2] = _fpga_cfg->rng;
	data[3] = _status;

	diagnosis_hexdump(_diag_print, DATA_TYPE_FPGA_CONFIG, (uint8_t *)data, sizeof(data));
}

void diagnosis_hexdump_mmc_cid(const diagnostic_print_s *_diag_print, uint32_t _result, uint8_t *_cid)
{
	uint8_t data[20];

	data[0] = (uint8_t)(_result);
	data[1] = (uint8_t)(_result >> 8);
	data[2] = (uint8_t)(_result >> 16);
	data[3] = (uint8_t)(_result >> 24);

	memcpy((uint8_t*)data + 4, _cid, 16);

	diagnosis_hexdump(_diag_print, DATA_TYPE_MMC_CID, data, sizeof(data));
}

void diagnosis_hexdump_device_type(const diagnostic_print_s *_diag_print, uint8_t _device_type)
{
	diagnosis_hexdump(_diag_print, DATA_TYPE_DEVICE, (uint8_t *)&_device_type, sizeof(uint8_t));
}

void diagnosis_hexdump_fw_version(const diagnostic_print_s *_diag_print, uint32_t _version)
{
	diagnosis_hexdump(_diag_print, DATA_TYPE_FIRMWARE_VERSION, (uint8_t *)&_version, sizeof(uint32_t));
}

void diagnosis_hexdump_bldr_version(const diagnostic_print_s *_diag_print, uint32_t _version)
{
	diagnosis_hexdump(_diag_print, DATA_TYPE_BOOTLOADER_VERSION, (uint8_t *)&_version, sizeof(uint32_t));
}

void diagnosis_hexdump_adc(const diagnostic_print_s *_diag_print, uint32_t _adc)
{
	diagnosis_hexdump(_diag_print, DATA_TYPE_ADC, (uint8_t *)&_adc, sizeof(uint32_t));
}

void diagnosis_hexdump_serial(const diagnostic_print_s *_diag_print, uint8_t *_serial)
{
	diagnosis_hexdump(_diag_print, DATA_TYPE_SERIAL_NUMBER, _serial, 16);
}