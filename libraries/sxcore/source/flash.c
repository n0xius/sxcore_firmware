#include "flash.h"
#include "gw_defines.h"

uint32_t flash_erase(uint32_t _page_address)
{
    fmc_unlock();

    fmc_state_enum state = fmc_page_erase(_page_address);

    fmc_lock();
    return state == FMC_READY ? GW_STATUS_FMC_SUCCESS : GW_STATUS_GENERIC_ERROR;
}

uint32_t flash_reprogram(uint8_t *_Dst, uint8_t *_Src, uint32_t _Size)
{
    fmc_unlock();

    uint32_t i = 0;

    for ( ; ; i += 4 )
    {
        if ( i >= _Size )
            break;

        if ( fmc_word_reprogram((uint32_t)(uint8_t*)_Dst + i, *(uint32_t*)((uint8_t*)_Src + i)) )
            break;
    }

    fmc_lock();
    return (i >= _Size) ? GW_STATUS_FMC_SUCCESS : GW_STATUS_GENERIC_ERROR;
}