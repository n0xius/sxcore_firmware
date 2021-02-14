#pragma once

#include <stdint.h>

// ---------------------------------------------------------
// Status codes
// ---------------------------------------------------------

#define GW_SUCCESS(x) ((uint16_t)((x) >> 24) == 0x900D)
#define GW_ERROR(x) ((uint16_t)((x) >> 24) == 0xBAD0)

#define GW_STATUS_SUCCESS ((uint32_t)0x900D0000)
#define GW_STATUS_FMC_SUCCESS ((uint32_t)0x900D0002)

#define GW_STATUS_RECEIVED_UPDATE_BLOCK ((uint32_t)0x900D0003)
#define GW_STATUS_SEND_OB_DATA ((uint32_t)0x900D0004)

#define GW_STATUS_GLITCH_SUCCESS ((uint32_t)0x900D0006) // glitch success
#define GW_STATUS_CONFIG_SUCCESS ((uint32_t)0x900D0007) // config loaded/written
#define GW_STATUS_BCT_PAYLOAD_SUCCESS ((uint32_t)0x900D0008) // bct & payload erased/written

#define GW_STATUS_GENERIC_ERROR ((uint32_t)0xBAD00000)

#define GW_STATUS_RESET ((uint32_t)0xBAD00002)
//#define GW_STATUS_ERROR_04 ((uint32_t)0xBAD00004)

#define GW_STATUS_AUTH_REQUIRED ((uint32_t)0xBAD00008)

#define GW_STATUS_FW_UPDATE_ERROR ((uint32_t)0xBAD00009)
#define GW_STATUS_FW_UPDATE_MAC_MISMATCH ((uint32_t)0xBAD0000A)

//#define GW_STATUS_ERROR_04 ((uint32_t)0xBAD0000E)

//#define GW_STATUS_ 0xBAD00012 // related to 0xFACE0041

#define GW_STATUS_NO_OR_UNKNOWN_DEVICE ((uint32_t)0xBAD00107) // no/unknown device (can happen if console is off) 
#define GW_STATUS_GLITCH_FAILED ((uint32_t)0xBAD00108) // failed to glitch

#define GW_STATUS_CONFIG_ERASE_ERROR ((uint32_t)0xBAD00109)
#define GW_STATUS_CONFIG_WRITE_ERROR ((uint32_t)0xBAD0010A)
#define GW_STATUS_CONFIG_INVALID_MAGIC ((uint32_t)0xBAD0010B) // invalid config magic

#define GW_STATUS_BCT_PAYLOAD_WRITE_ERROR ((uint32_t)0xBAD0010C) // write_bct_and_payload/reset_bct_and_erase_payload failed

// mmc_send_cmd
#define GW_STATUS_MMC_GO_IDLE_STATE_FAILED ((uint32_t)0xBAD0010D) // MMC_GO_IDLE_STATE failed

#define GW_STATUS_MMC_SEND_OP_COND_FAILED ((uint32_t)0xBAD0010E) // MMC_SEND_OP_COND failed
#define GW_STATUS_MMC_SEND_OP_COND_TIMEOUT ((uint32_t)0xBAD00110) // MMC_SEND_OP_COND timeout

#define GW_STATUS_MMC_ALL_SEND_CID_FAILED ((uint32_t)0xBAD00111) // MMC_ALL_SEND_CID failed

#define GW_STATUS_MMC_SET_RELATIVE_ADDR_FAILED ((uint32_t)0xBAD00112) // MMC_SET_RELATIVE_ADDR failed
#define GW_STATUS_MMC_SET_RELATIVE_ADDR_RESPONSE ((uint32_t)0xBAD00113) // MMC_SET_RELATIVE_ADDR response error

#define GW_STATUS_MMC_SEND_CSD_FAILED ((uint32_t)0xBAD00114) // MMC_SEND_CSD failed

#define GW_STATUS_MMC_SELECT_CARD_FAILED ((uint32_t)0xBAD00115) // MMC_SELECT_CARD failed
#define GW_STATUS_MMC_SELECT_CARD_RESPONSE ((uint32_t)0xBAD00116) // MMC_SELECT_CARD response error

#define GW_STATUS_MMC_SEND_STATUS_FAILED ((uint32_t)0xBAD00117) // MMC_SEND_STATUS failed
#define GW_STATUS_MMC_SEND_STATUS_RESPONSE ((uint32_t)0xBAD00118) // MMC_SEND_STATUS response error

#define GW_STATUS_MMC_SET_BLOCKLEN_FAILED ((uint32_t)0xBAD00119) // MMC_SET_BLOCKLEN failed
#define GW_STATUS_MMC_SET_BLOCKLEN_RESPONSE ((uint32_t)0xBAD0011A) // MMC_SET_BLOCKLEN response error

#define GW_STATUS_MMC_SWITCH_FAILED ((uint32_t)0xBAD0011B) // MMC_SWITCH failed
#define GW_STATUS_MMC_SWITCH_RESPONSE ((uint32_t)0xBAD0011C) // MMC_SWITCH response error

#define GW_STATUS_MMC_READ_SINGLE_BLOCK_FAILED ((uint32_t)0xBAD0011D) // failed to read mmc block
#define GW_STATUS_MMC_READ_SINGLE_BLOCK_RESPONSE ((uint32_t)0xBAD0011E) // mmc read error

#define GW_STATUS_MMC_WRITE_BLOCK_FAILED ((uint32_t)0xBAD00120) // failed to write mmc block
#define GW_STATUS_MMC_WRITE_BLOCK_RESPONSE ((uint32_t)0xBAD00121) // mmc write error

// glitch process
#define GW_STATUS_ADC_CHANNEL_READ_MISMATCH ((uint32_t)0xBAD00122) // no mmc communication

#define GW_STATUS_GLITCH_TIMEOUT ((uint32_t)0xBAD00124) // some glitch setup error

#define GW_STATUS_CONFIG_FPGA_OVERFLOW ((uint32_t)0xBAD00125)

// ---------------------------------------------------------
// USB Commands
// ---------------------------------------------------------

#define DIAGNOSIS_BOOT ((uint8_t)'b')
#define DIAGNOSIS_RUN_DIAGNOSIS ((uint8_t)'d')
#define DIAGNOSIS_ERASE_EMMC_PAYLOAD ((uint8_t)'e')
#define DIAGNOSIS_PROGRAM_EMMC_PAYLOAD ((uint8_t)'p')
#define DIAGNOSIS_FACTORY_RESET ((uint8_t)'r')
#define DIAGNOSIS_TOGGLE_CHIP ((uint8_t)'s')
#define DIAGNOSIS_RUN_TEST ((uint8_t)'t')
#define DIAGNOSIS_GET_CHIP_INFO ((uint8_t)'v')
#define DIAGNOSIS_FPGA_INCREASE_WIDTH ((uint8_t)'[')
#define DIAGNOSIS_FPGA_DECREASE_WIDTH ((uint8_t)']')
#define DIAGNOSIS_FPGA_INCREASE_RNG ((uint8_t)'<')
#define DIAGNOSIS_FPGA_DECREASE_RNG ((uint8_t)'>')
#define DIAGNOSIS_FPGA_INCREASE_OFFSET ((uint8_t)'+')
#define DIAGNOSIS_FPGA_DECREASE_OFFSET ((uint8_t)'-')

// ---------------------------------------------------------

#define CMD_GET_OB_PROTECTION ((uint32_t)0x0D15EA5E) // get ob protection data
#define CMD_SET_OB_PROTECTION ((uint32_t)0x0DEFACED) // set ob protection data (authentication required)

#define CMD_PING ((uint32_t)0xFACE0000) // ping
#define CMD_INIT_FW_UPDATE ((uint32_t)0xFACE0002) // fw update (authentication required)
#define CMD_FW_UPDATE_BUFFER_POS ((uint32_t)0xFACE0004) // get current update buffer position
#define CMD_FW_UPDATE_PACKET_CHECKSUM ((uint32_t)0xFACE0005) // get amount of update_packets recieved
#define CMD_GET_UNIQUE_CHIP_ID ((uint32_t)0xFACE0006) // read the unique id from SYSCFG 1FFFF7AC
#define CMD_SET_LED_COLOR ((uint32_t)0xFACE0008) // set led color
#define CMD_SPI0_INITALIZATION ((uint32_t)0xFACE0009) // spi0 init with SPI_PSC_4
#define CMD_AUTHENTICATE ((uint32_t)0xFACE000F) // authenticate (see AUTHENTICATION_KEY_1, AUTHENTICATION_KEY_2)
#define CMD_GET_SERIAL_NUMBER ((uint32_t)0xFACE0010) // get sx core/lite serial number
#define CMD_SET_SERIAL_NUMBER ((uint32_t)0xFACE0011) // set sx core/lite serial number (authentication required, only can be set if not set already)
#define CMD_RESET_FPGA ((uint32_t)0xFACE0020) // reset fpga, initalize mmc (argument 1: 0xAABBCCDD) (authentication required)
#define CMD_SEND_FPGA_CMD ((uint32_t)0xFACE0021) // send fpga command (authentication required)
#define CMD_SEND_MMC_CMD ((uint32_t)0xFACE0022) // send mmc command (authentication required)
#define CMD_WRITE_MMC_BLOCK ((uint32_t)0xFACE0023) // write mmc block (authentication required)
#define CMD_READ_MMC_BLOCK ((uint32_t)0xFACE0024) // read mmc block (authentication required)
#define CMD_COPY_MMC_BLOCK ((uint32_t)0xFACE0025) // copy mmc block (authentication required)
#define CMD_SET_FPGA_DATA_TYPE ((uint32_t)0xFACE0026) // set spi0 data type (authentication required)
#define CMD_SEND_FPGA_CMD_1 ((uint32_t)0xFACE0027) // send fpga command (authentication required)
#define CMD_GET_SPI0_STATUS ((uint32_t)0xFACE0029) // get spi0 status (authentication required)
//#define CMD_SEND_SPI0_DATA_24 ((uint32_t)0xFACE002A) // send data over spi0 via 0x24 (authentication required)
//#define CMD_XFER_SPI0_DATA_26 ((uint32_t)0xFACE002B) // transfer data over spi0 via 0x26 (authentication required)
#define CMD_GET_BOOTLDR_MODE ((uint32_t)0xFACE0031) // get bootloader debug mode value (see BOOTLOADER_DEBUG_MODE, BOOTLOADER_RETAIL_MODE)
#define CMD_GET_FW_VERSION ((uint32_t)0xFACE0032) // get firmware version
#define CMD_SET_BLDR_VERSION ((uint32_t)0xFACE0033) // set bootloader version (authentication required)
#define CMD_GET_BLDR_VERSION ((uint32_t)0xFACE0034) // get bootloader version
//#define CMD_ ((uint32_t)0xFACE0036) // TODO: figure out (authentication required)
//#define CMD_ ((uint32_t)0xFACE003B) // TODO: figure out (authentication required)
//#define CMD_ ((uint32_t)0xFACE003C) // TODO: figure out (authentication required)
//#define CMD_ ((uint32_t)0xFACE003D) // TODO: figure out (authentication required)
#define CMD_GET_BOARD_ID ((uint32_t)0xFACE003E) // get board type by pin (authentication required)
#define CMD_ ((uint32_t)0xFACE003F) // spi1 setup, send 0x57, recv 0x8F, recv 0xE8 (authentication required)
#define CMD_RESET_FPGA_SETTINGS ((uint32_t)0xFACE0040) // reset fpga settings? (authentication required)
//#define CMD_ ((uint32_t)0xFACE0041) // TODO: figure out (authentication required)

// ---------------------------------------------------------
// Miscellaneous
// ---------------------------------------------------------

#define CONFIG_MAGIC 				((uint32_t)0x786616E2)
#define CHIP_MAGIC					((uint32_t)0xF0760421)

#define BOOTLOADER_DEBUG_MODE 		((uint32_t)0xE81A39CD)
#define BOOTLOADER_RETAIL_MODE 		((uint32_t)0xFFFFFFFF)

#define GW_AUTH_MASTER_KEY 			(uint8_t*)"\xEE\x39\x25\x0E\xF9\x07\x4D\xF8\x7A\xC7\x5F\x88"
#define GW_AUTH_PROGRAM_KEY 		(uint8_t*)"\x02\x35\x29\x45\xFC\x1D\xEA\x20\x4C\xD9\x06\x5B"

#define GW_TEA_KEY_1 				((uint32_t)0x73707048)
#define GW_TEA_KEY_2 				((uint32_t)0xE8AD34FB)
#define GW_TEA_KEY_3 				((uint32_t)0xA4A8ACE6)
#define GW_TEA_KEY_4 				((uint32_t)0x7B648B68)

#define GW_FW_UPDATE_MAX_SIZE 		((uint32_t)0x1CC00)
#define GW_FLASH_MAX_SIZE 			((uint32_t)0x20000)

#define GW_USBD_VID 				((uint16_t)0xC001)
#define GW_USBD_PID 				((uint16_t)0xC0DE)

#ifndef NVIC_VECTTAB_FLASH
#define NVIC_VECTTAB_FLASH          ((uint32_t)0x08000000)
#endif

#ifndef NVIC_VECTTAB_RAM
#define NVIC_VECTTAB_RAM            ((uint32_t)0x20000000)
#endif // NVIC_VECTTAB_RAM

// ---------------------------------------------------------
// Structs, enums and function templates
// ---------------------------------------------------------

typedef enum _authentication_type
{
    AUTH_NO_KEY,
    AUTH_PROGRAM_KEY,
    AUTH_MASTER_KEY
} authentication_type;

typedef enum _device_type
{
    DEVICE_TYPE_UNKNOWN = 0,
	DEVICE_TYPE_ERISTA,
	DEVICE_TYPE_MARIKO,
	DEVICE_TYPE_LITE
} device_type;

typedef struct _bootloader_usb_s
{
    uint8_t *recv_buffer;
    uint8_t *send_buffer;
    uint32_t (*recv_func)(void);
    void (*send_func)(uint32_t data_len);
} bootloader_usb_s;

typedef struct _firmware_header_s
{
	uint32_t buffer_pos;
	uint32_t size;
	uint32_t version;
	uint32_t erase_flash;
	uint8_t mac[16];
} firmware_header_s;

typedef struct _firmware_update_block_s
{
	uint8_t data[64];
} firmware_update_block_s;

typedef struct dfu_command_no_arg_s
{
	uint32_t cmd;
} dfu_command_no_arg;

typedef struct dfu_command_one_arg_s
{
	uint32_t cmd;
	uint32_t argument;
} dfu_command_one_arg, dfu_command_fpga_cmd;

typedef struct dfu_command_two_args_s
{
	uint32_t cmd;
	uint32_t argument1;
	uint32_t argument2;
} dfu_command_two_args, dfu_command_mmc_cmd;

typedef struct dfu_command_xfer_mmc_s
{
	uint32_t cmd;
	uint32_t block_index;
	uint8_t data[32];
} dfu_command_xfer_mmc_s;

typedef struct dfu_command_s
{
	uint32_t cmd;
	uint32_t argument;
	uint8_t data[56];
} dfu_command; // limited to the usb receive buffer size

typedef struct fpga_data_header_s
{
	uint8_t status;
	uint8_t width;
	uint16_t offset;
	uint8_t data_26_0A;
	uint8_t data_size;
	uint8_t did_save_config;
	uint8_t rng;
} fpga_data_header;

typedef struct fpga_data_s
{
	uint8_t data[48];
} fpga_data;

typedef struct mmc_cid_s
{
	uint32_t status;
	uint8_t cid[16];
} mmc_cid;

typedef struct _config_s
{
	uint32_t magic;
	uint32_t pad04;
	uint32_t saved_glitch_data;
	uint16_t offsets[32];
	uint8_t width[32];
	uint32_t chip_enabled;
} config_s;

typedef struct _fpga_config_s
{
	int32_t width;
	uint32_t offset;
	int32_t rng;
} fpga_config_s;

typedef struct _timeout_s
{
    uint32_t update_time;
    uint32_t dword4;
    uint64_t total_time_passed;
} timeout_s;

typedef struct _spi_parser_s
{
  uint8_t* buffer;
  uint32_t buffer_length;
  uint8_t *orig_buffer;
  uint8_t datatype;
  uint8_t cmd;
  uint32_t args;
} spi_parser_s;

typedef enum
{
	DATA_TYPE_FPGA_START = 1, // 0 bytes
	DATA_TYPE_FPGA_HEADER = 2, // 8 bytes
	DATA_TYPE_FPGA = 3, // maximum of 48 bytes
	DATA_TYPE_CONFIG = 4, // 112 bytes, fpga_config (v1.1: 108 bytes; v1.3: 112 bytes)
	DATA_TYPE_FPGA_CONFIG = 5, // 16 bytes, fpga_config
	DATA_TYPE_MMC_CID = 6, // 20 bytes, mmc_cid
	DATA_TYPE_DEVICE = 7, // 1 byte
	DATA_TYPE_FIRMWARE_VERSION = 8, // 4 bytes
	DATA_TYPE_BOOTLOADER_VERSION = 9, // 4 bytes
	DATA_TYPE_ADC = 10, // 4 bytes
	DATA_TYPE_SERIAL_NUMBER = 11, // 16 bytes
} diagnosis_data_type;

typedef struct _diagnostic_print_s
{
	void (*begin)();
	void (*hexdump)(uint8_t, uint8_t *, uint32_t);
	void (*end)();
} diagnostic_print_s;

typedef void(*void_function_t)(void);
typedef uint32_t(*handle_usb_firmware_command_t)(bootloader_usb_s*, uint32_t, uint8_t);