#include "gd32f3x0_it.h"

#include "gw_defines.h"
#include "firmware_update.h"
#include "flash.h"
#include "tea.h"
#include "usb.h"

uint32_t firmware_update_size = 0;
uint32_t firmware_update_buffer_position = 0;
uint32_t firmware_update_version = 0;
uint32_t firmware_update_block_checksum = 0;
uint32_t firmware_should_erase = 0;

uint8_t firmware_mac[16];
uint8_t firmware_update_mac[16];

const uint32_t tea_key[4] = { GW_TEA_KEY_1, GW_TEA_KEY_2, GW_TEA_KEY_3, GW_TEA_KEY_4 };

void reset_function_table() {
    g_ram_function_table = 0;
}

uint32_t get_firmware_buffer_position(void){
    return firmware_update_buffer_position;
}

uint32_t get_firmware_block_checksum(void){
    return firmware_update_block_checksum;
}

uint32_t validate_firmware_header(uint8_t* _firmware_buffer, uint32_t _should_erase)
{
    firmware_header_s* firmware_header = (firmware_header_s*)_firmware_buffer;

    firmware_update_size = 0;

    firmware_update_buffer_position = REG32(firmware_header->buffer_pos);
    firmware_update_size = REG32(firmware_header->size);
    firmware_update_version = REG32(firmware_header->version);

    firmware_should_erase = _should_erase;

    memset(firmware_mac, 0, sizeof(firmware_mac));

    memcpy(firmware_update_mac, firmware_header->mac, sizeof(firmware_update_mac));

    reset_function_table();

    if (firmware_update_size > GW_FLASH_MAX_SIZE
        || firmware_update_size > GW_FLASH_MAX_SIZE
        || (((uint16_t)firmware_update_buffer_position | (uint16_t)firmware_update_size) & 0x3FF) | firmware_update_buffer_position)
    {
        firmware_update_size = 0;
        firmware_update_buffer_position = 0;
        return GW_STATUS_FW_UPDATE_ERROR; //0xBAD00009
    }
    else if(!firmware_should_erase || flash_erase((uint32_t)&__firmware) == GW_STATUS_FMC_SUCCESS) {
        return GW_STATUS_FMC_SUCCESS; //0x900D0002
    }

    firmware_update_size = 0;
    firmware_update_buffer_position = 0;
    return GW_STATUS_GENERIC_ERROR; //0xBAD00000
}

uint32_t handle_firmware_update(uint8_t* _firmware_buffer)
{
    if (!firmware_update_size
        || firmware_update_buffer_position > GW_FW_UPDATE_MAX_SIZE)
        return GW_STATUS_FW_UPDATE_ERROR; //0xBAD00009

    if (!(firmware_update_buffer_position & 0x3FF)
        && firmware_should_erase
        && flash_erase((uint32_t)&__firmware + firmware_update_buffer_position) != GW_STATUS_FMC_SUCCESS)
        return GW_STATUS_GENERIC_ERROR; //0xBAD00000

    firmware_update_block_s update_block;
    memcpy((uint8_t*)&update_block, _firmware_buffer, sizeof(update_block));

    for (uint32_t i = 0; i != sizeof(firmware_update_block_s); i += 8)
    {
        uint32_t key_with_offset[4];
        tea_get_key_with_offset((uint32_t*)tea_key, key_with_offset, firmware_update_buffer_position + i);

        tea_decrypt(key_with_offset, (uint32_t*)update_block.data);

        tea_update_custom_mac((uint32_t*)update_block.data, key_with_offset, (uint32_t*)firmware_mac);
    }

    uint8_t should_initialize = 0;
    if (firmware_update_buffer_position == 0x1C0)
    {
        REG32(((uint8_t*)&update_block.data) + 0x3C) = (uint32_t)0xFFFFFFFF;
        should_initialize = 1;
    }

    if ( firmware_should_erase
         && flash_reprogram((uint8_t *)((uint32_t)&__firmware + firmware_update_buffer_position),
                            (uint8_t *)update_block.data,
                            sizeof(firmware_update_block_s)) != GW_STATUS_FMC_SUCCESS)
    {
        return GW_STATUS_GENERIC_ERROR; //0xBAD00000
    }

    firmware_update_buffer_position += sizeof(firmware_update_block_s);
    firmware_update_size -= sizeof(firmware_update_block_s);

    if (firmware_update_size)
        return GW_STATUS_RECEIVED_UPDATE_BLOCK; //0x900D0003

    if (memcmp(firmware_update_mac, firmware_mac, sizeof(firmware_mac)) != 0)
        return GW_STATUS_FW_UPDATE_MAC_MISMATCH; //0xBAD0000A

    if (!should_initialize)
        return GW_STATUS_SUCCESS; //0x900D0000

    uint32_t* g_firmware_version = (uint32_t*)((uint32_t)&__firmware + 0x158);

    if ( firmware_should_erase
         && flash_reprogram((uint8_t *)g_firmware_version, (uint8_t *)&firmware_update_version, sizeof(uint32_t)) != GW_STATUS_FMC_SUCCESS )
    {
        return GW_STATUS_GENERIC_ERROR; //0xBAD00000
    }

    uint32_t* g_did_initialize_functions = (uint32_t*)((uint32_t)&__firmware + 0x1FC);

    uint32_t value = 0;
    if ( firmware_should_erase
         && flash_reprogram((uint8_t *)g_did_initialize_functions, (uint8_t *)&value, sizeof(uint32_t)) != GW_STATUS_FMC_SUCCESS )
    {
        return GW_STATUS_GENERIC_ERROR; //0xBAD00000
    }

    return GW_STATUS_SUCCESS; //0x900D0000
}