#include "gd32f3x0_it.h"

#include <usb_core.h>
#include <usbd_int.h>

extern usb_core_handle_struct bootloader_usb_core;
extern void main();

/*!
	\brief	  this function handles device resets
	\param[in]  none
	\param[out] none
	\retval	 none
*/
void reset_handler(void)
{
    SystemInit();

    // NOTE: copy data into ram
    memcpy(&__bss_start__, &__data_start__, (uint32_t)&__data_size__);

    // NOTE: clear stack
    memset(&__bss_start__, 0, (uint32_t)&__bss_size__);

    main();
}

/*!
	\brief	  this function handles USBD interrupt
	\param[in]  none
	\param[out] none
	\retval	 none
*/
void usbfs_irq_handler(void)
{
	usbd_isr(&bootloader_usb_core);
}

int memcmp ( const void* _Ptr1, const void* _Ptr2, uint32_t _Size )
{
    for(uint32_t i = 0; i < _Size; ++i)
    {
        if (((uint8_t*)_Ptr1)[i] != ((uint8_t*)_Ptr2)[i])
            return ((uint8_t*)_Ptr1)[i] > ((uint8_t*)_Ptr2)[i] ? -i : i;
    }

    return 0;
}

void* memcpy ( void* _Dst, const void* _Src, uint32_t _Size )
{
    for(uint32_t i = 0; i < _Size; ++i)
        ((uint8_t*)_Dst)[i] = ((uint8_t*)_Src)[i];

    return _Dst;
}

void* memset ( void* _Dst, int _Val, uint32_t _Size )
{
    for(uint32_t i = 0; i < _Size; ++i)
        ((uint8_t*)_Dst)[i] = (uint8_t)_Val;

    return _Dst;
}