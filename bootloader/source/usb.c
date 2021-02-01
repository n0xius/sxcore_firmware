#include <usbd_core.h>

#include "gd32f3x0_it.h"
#include "gw_cdc_acm_core.h"
#include "gw_defines.h"

#include "usb.h"
#include "delay.h"
#include "flash.h"
#include "rgb_led.h"
#include "firmware_update.h"

uint32_t* g_ram_function_table = 0;

uint32_t usbfs_prescaler = 0U;

uint8_t usb_recv_buffer[CDC_ACM_DATA_PACKET_SIZE];
uint8_t usb_send_buffer[CDC_ACM_DATA_PACKET_SIZE];

uint8_t is_authenticated = AUTH_NO_KEY;

uint8_t* auth_master_key = GW_AUTH_MASTER_KEY;
uint8_t* auth_program_key = GW_AUTH_PROGRAM_KEY;

usb_core_handle_struct bootloader_usb_core =
        {
                .dev =
                        {
                                .dev_desc = (uint8_t *)&device_descriptor,
                                .config_desc = (uint8_t *)&configuration_descriptor,
                                .strings = usbd_strings,
                                .class_init = cdc_acm_init,
                                .class_deinit = cdc_acm_deinit,
                                .class_req_handler = cdc_acm_req_handler,
                                .class_data_handler = cdc_acm_data_handler
                        },

                .udelay = delay_us,
                .mdelay = delay_ms
        };

uint32_t ob_set_protection(uint16_t _spc)
{
    fmc_unlock();
    ob_unlock();

    uint32_t status = ob_erase() || ob_security_protection_config((uint8_t)_spc) ? GW_STATUS_GENERIC_ERROR : GW_STATUS_FMC_SUCCESS;

    ob_lock();
    fmc_lock();
    return status;
}

void initialize_usb()
{
    /*// NOTE: the usb clock initialization is duplicated for whatever reason?!
    {
        // NOTE: replaced "usbfs_prescaler = RCU_USBFS_CKPLL_DIV2" with proper code below for support of dynamic clocks
        switch (rcu_clock_freq_get(CK_SYS)) {
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
    }*/

    nvic_irq_enable(USBFS_IRQn, 2, 0);

    usbd_init (&bootloader_usb_core, USB_FS_CORE_ID);
}

void wait_for_usb_configuration()
{
    while(bootloader_usb_core.dev.status != USB_STATUS_CONFIGURED);
}

uint32_t usb_recv_func(void)
{
    cdc_acm_data_receive(&bootloader_usb_core);

    // NOTE: wait until packet was fully received
    while(packet_receive);

    return receive_length;
}

void usb_send_func(uint32_t _data_len)
{
    cdc_acm_data_send(&bootloader_usb_core, _data_len);

    // NOTE: wait until packet was fully sent
    while(!packet_sent);
}

void usb_send_dword(uint32_t _data)
{
    *(uint32_t*)usb_send_buffer = _data;

    usb_send_func(sizeof(uint32_t));
}

uint32_t bootloader_handle_usb_command()
{
    uint32_t cmd = REG32(usb_recv_buffer);
    uint32_t status = 0;

    switch(cmd)
    {
        case CMD_PING:                              // 0xFACE0000
        {
            status = GW_STATUS_SUCCESS; // 0x900D0000
            break;
        }

        case CMD_INIT_FW_UPDATE:                    // 0xFACE0002
        {
            if (is_authenticated == AUTH_NO_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            status = validate_firmware_header((uint8_t *)(usb_recv_buffer + 4), 1);
            break;
        }

        case CMD_FW_UPDATE_BUFFER_POS:              // 0xFACE0004
        {
            status = get_firmware_buffer_position();
            break;
        }

        // NOTE: used inside sxupdater_win32/sxflasher to verify if the block was read correctly
        case CMD_FW_UPDATE_PACKET_CHECKSUM:         // 0xFACE0005
        {
            // NOTE: calculated in handle_firmware_update
            status = get_firmware_block_checksum();
            break;
        }

        // NOTE:  something with the mcu's serial
        case 0xFACE0006:
        {
            usb_send_buffer[0] = 0;
            usb_send_buffer[1] = 0xDE;
            usb_send_buffer[2] = 0xC0;
            usb_send_buffer[3] = 0x1D;

            uint8_t* unique_chip_id = (uint8_t*)(SYSMEM_BASE + 0xBAC); // 0x1FFFF7AC

            usb_send_buffer[4] = unique_chip_id[0];
            usb_send_buffer[5] = unique_chip_id[1];
            usb_send_buffer[6] = unique_chip_id[2];
            usb_send_buffer[7] = unique_chip_id[3];

            // NOTE: this is actually just 8 times 0xFF since team xecuter managed to use the wrong address lol
            /*uint8_t* other_data = (uint8_t*)(SYSMEM_BASE + 0xBD0); // 0x1FFFF7D0

            usb_send_buffer[8] = other_data[0];
            usb_send_buffer[9] = other_data[1];
            usb_send_buffer[10] = other_data[2];
            usb_send_buffer[11] = other_data[3];
            usb_send_buffer[12] = other_data[5];
            usb_send_buffer[13] = other_data[6];
            usb_send_buffer[14] = other_data[7];
            usb_send_buffer[15] = other_data[8];*/

            usb_send_buffer[8] = unique_chip_id[4];
            usb_send_buffer[9] = unique_chip_id[5];
            usb_send_buffer[10] = unique_chip_id[6];
            usb_send_buffer[11] = unique_chip_id[7];
            usb_send_buffer[12] = unique_chip_id[8];
            usb_send_buffer[13] = unique_chip_id[9];
            usb_send_buffer[14] = unique_chip_id[10];
            usb_send_buffer[15] = unique_chip_id[11];

            usb_send_func(16);
            break;
        }

        case CMD_SET_LED_COLOR:                     // 0xFACE0008
        {
            uint32_t led_num = REG32(usb_recv_buffer[4]);
            uint32_t led_clr = REG32(usb_recv_buffer[8]);

            switch(led_num)
            {
                case 1:
                    set_led_color_green((uint8_t)led_clr);
                    break;
                case 2:
                    set_led_color_blue((uint8_t)led_clr);
                    break;
                case 3:
                    set_led_color_red((uint8_t)led_clr);
                    break;
                default:
                    break;
            }

            status = GW_STATUS_SUCCESS;
            break;
        }

        case CMD_AUTHENTICATE:                      // 0xFACE000F
        {
            if (!memcmp((uint8_t *)(usb_recv_buffer + 4), auth_master_key, 12))
                is_authenticated = AUTH_MASTER_KEY;
            else if (!memcmp((uint8_t *)(usb_recv_buffer + 4), auth_program_key, 12))
                is_authenticated = AUTH_PROGRAM_KEY;
            else
                is_authenticated = AUTH_NO_KEY;

            status = GW_STATUS_SUCCESS;
            break;
        }

        case CMD_GET_SERIAL_NUMBER:                      // 0xFACE0010
        {
            uint8_t* g_serial_number = (uint8_t*)((uint32_t)&__bootloader + 0x150);

            for(uint32_t i = 0; i < 16; ++i)
                usb_send_buffer[i] = g_serial_number[i];

            usb_send_func(sizeof(g_serial_number));
            break;
        }

        case CMD_SET_SERIAL_NUMBER:                     // 0xFACE0011
        {
            if (is_authenticated != AUTH_MASTER_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint8_t* g_serial_number = (uint8_t*)((uint32_t)&__bootloader + 0x150);

            // NOTE: one time programming of the serial number
            if ( *(uint32_t*)((uint8_t*)g_serial_number + 0) != 0xFFFFFFFF
                 || *(uint32_t *)((uint8_t*)g_serial_number + 4) != 0xFFFFFFFF
                 || *(uint32_t *)((uint8_t*)g_serial_number + 8) != 0xFFFFFFFF
                 || *(uint32_t *)((uint8_t*)g_serial_number + 12) != 0xFFFFFFFF )
            {
                status = GW_STATUS_FW_UPDATE_ERROR; // 0xBAD00009
                break;
            }

            status = flash_reprogram(g_serial_number, (uint8_t *)(usb_recv_buffer + 4), 16);
            break;
        }

        case CMD_GET_BOOTLDR_MODE:                  // 0xFACE0031
        {
            uint32_t* g_debug_mode = (uint32_t*)((uint32_t)&__bootloader + 0x168);
            usb_send_dword(*g_debug_mode);
            break;
        }

        case CMD_GET_FW_VERSION:                    // 0xFACE0032
        {
            uint32_t* g_firmware_version = (uint32_t*)((uint32_t)&__firmware + 0x158);
            usb_send_dword(*g_firmware_version);
            break;
        }

        case CMD_SET_BLDR_VERSION:                   // 0xFACE0033
        {
            if (is_authenticated != AUTH_MASTER_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint32_t* g_bootloader_version = (uint32_t*)((uint32_t)&__bootloader + 0x160);
            status = flash_reprogram((uint8_t *)g_bootloader_version, (uint8_t *)(usb_recv_buffer + 4), sizeof(uint32_t));
            break;
        }

        case CMD_GET_BLDR_VERSION:                   // 0xFACE0034
        {
            uint32_t* g_bootloader_version = (uint32_t*)((uint32_t)&__bootloader + 0x160);
            usb_send_dword(*g_bootloader_version);
            break;
        }

        case CMD_GET_OB_PROTECTION:                 // 0x0D15EA5E
        {
            usb_send_dword(GW_STATUS_SEND_OB_DATA); // 0x900D0004
            usb_send_dword(*(uint32_t*)(OB + 0));   // OB_SPC & OB_USER
            usb_send_dword(*(uint32_t*)(OB + 4)); // OB_DATA0 & OB_DATA1
            usb_send_dword(*(uint32_t*)(OB + 8));   // OB_WP0 & OB_WP1
            break;
        }

        case CMD_SET_OB_PROTECTION:                 // 0x0DEFACED
        {
            if (is_authenticated != AUTH_MASTER_KEY)
            {
                status = GW_STATUS_AUTH_REQUIRED;
                break;
            }

            uint16_t spc_value = (uint16_t)(REG32(usb_recv_buffer[4]) & 0xFFFF);

            status = ob_set_protection(spc_value);
            break;
        }

        default:
            return ((uint32_t)0xFFFFFFFF);
    }

    if (status != 0)
        usb_send_dword(status);

    return 0;
}

int32_t initialize_ram_func_table_and_run_firmware_cmd(bootloader_usb_s *_bootloader_usb, uint32_t _usb_cmd_size, uint8_t _is_authenticated)
{
    uint32_t* g_debug_mode = (uint32_t*)((uint32_t)&__bootloader + 0x168);
    uint32_t* g_did_initialize_functions = (uint32_t*)((uint32_t)&__firmware + 0x1FC);

    if( (*g_debug_mode != BOOTLOADER_DEBUG_MODE && *g_did_initialize_functions) )
        return ((int32_t)-1);

    if (!g_ram_function_table)
    {
        uint32_t* g_flash_function_table = (uint32_t*)((uint32_t)&__firmware + 0x150);
        g_ram_function_table = g_flash_function_table;

        // call firmware_reset
        ((reset_function_t)g_ram_function_table[0])();
    }

    // call firmware_handle_usb_command
    return ((handle_usb_firmware_command_t)g_ram_function_table[1])(_bootloader_usb, _usb_cmd_size, _is_authenticated);
}

void handle_usb_transfers()
{
    bootloader_usb_s bootloader_usb;

    bootloader_usb.recv_buffer = usb_recv_buffer;
    bootloader_usb.send_buffer = usb_send_buffer;
    bootloader_usb.recv_func = usb_recv_func;
    bootloader_usb.send_func = usb_send_func;

    initialize_usb();

    while (1)
    {
        uint32_t status_code = 0;
        uint32_t recv_data_size = 0;

        do{
            wait_for_usb_configuration();
            recv_data_size = usb_recv_func();
        } while(!recv_data_size);

        if (recv_data_size == sizeof(firmware_update_block_s))
        {
            status_code = handle_firmware_update(usb_recv_buffer);

            usb_send_dword(status_code);
        }
        else if (recv_data_size <= 3 || bootloader_handle_usb_command())
        {
            status_code = initialize_ram_func_table_and_run_firmware_cmd(&bootloader_usb, recv_data_size, is_authenticated);

            if (status_code <= 0)
            {
                if (status_code != 0)
                {
                    status_code = 0xBAD00002;
                    usb_send_dword(status_code);
                }
            }
            else
                usb_send_func(status_code);
        }
    }
}