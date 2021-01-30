#include "gd32f3x0_it.h"

#include "gw_defines.h"
#include "configuration.h"
#include "flash.h"

config_s* g_saved_config = (config_s*)((uint32_t)&__config);

fpga_config_s g_fpga_config = {
        50, 800, 0
};

extern int memcmp ( const void* _Ptr1, const void* _Ptr2, uint32_t _Size );
extern void* memcpy ( void* _Dst, const void* _Src, uint32_t _Size );
extern void* memset ( void* _Dst, int _Val, uint32_t _Size );

void config_clear(config_s *_config)
{
    memset(&_config->offsets, 0xFF, 100);

    _config->magic = 0;
    _config->pad04 = 0;
    _config->saved_glitch_data = 0;
}

uint8_t config_is_chip_enabled(config_s *_config)
{
    return _config->chip_enabled == CHIP_MAGIC;
}

void config_set_chip_enabled(config_s *_config, uint8_t _chip_enabled)
{
    _config->chip_enabled = _chip_enabled ? CHIP_MAGIC : 0;
}

uint32_t config_save_fpga_cfg(config_s *_config, fpga_config_s *_fpga_cfg)
{
    uint32_t saved_fpga_data = _config->saved_glitch_data;

    if ( saved_fpga_data > 31 )
        return GW_STATUS_CONFIG_FPGA_OVERFLOW; // 0xBAD00125

    _config->offsets[saved_fpga_data] = _fpga_cfg->offset;
    _config->width[saved_fpga_data] = _fpga_cfg->width;
    _config->saved_glitch_data = saved_fpga_data + 1;

    return GW_STATUS_CONFIG_SUCCESS; // 0x900D0007
}

uint32_t config_load_from_flash(config_s *_config)
{
    // NOTE: linker cries about 4 byte alignment reading in a 4 aligned section
    //       meanwhile the g_saved_config location is aligned by 1024 for fmc paging
    memcpy(_config, g_saved_config, sizeof(config_s));

    if ( _config->magic != CONFIG_MAGIC )
    {
        config_clear(_config);
        return GW_STATUS_CONFIG_INVALID_MAGIC; // 0xBAD0010B
    }

    for ( uint32_t i = 0; i != 32; ++i )
    {
        if ( _config->width[i] == 0xFF || _config->offsets[i] == 0xFFFF )
        {
            _config->saved_glitch_data = i;
            break;
        }
    }

    return GW_STATUS_CONFIG_SUCCESS; // 0x900D0007
}

uint32_t config_write_to_flash(config_s *_config)
{
    _config->magic = CONFIG_MAGIC;

    if ( flash_erase((uint32_t)g_saved_config) != GW_STATUS_FMC_SUCCESS )
        return GW_STATUS_CONFIG_ERASE_ERROR; // 0xBAD00109

    if ( flash_reprogram((uint8_t*)g_saved_config, (uint8_t*)_config, sizeof(config_s)) != GW_STATUS_FMC_SUCCESS )
        return GW_STATUS_CONFIG_WRITE_ERROR; // 0xBAD0010A

    return GW_STATUS_CONFIG_SUCCESS; // 0x900D0007
}

uint8_t is_chip_disabled(void)
{
    config_s cfg;

    if ( config_load_from_flash(&cfg) == GW_STATUS_CONFIG_SUCCESS )
        return config_is_chip_enabled(&cfg);

    return 0;
}