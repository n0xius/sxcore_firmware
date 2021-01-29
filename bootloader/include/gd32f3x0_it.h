#pragma once

#include <gd32f3x0.h>

/* this function handles device resets */
void reset_handler(void);
/* this function handles USBFS IRQ Handler */
void usbfs_irq_handler(void);

extern int memcmp ( const void* _Ptr1, const void* _Ptr2, uint32_t _Size );
extern void* memcpy ( void* _Dst, const void* _Src, uint32_t _Size );
extern void* memset ( void* _Dst, int _Val, uint32_t _Size );

// defined in linker.ld
extern uint32_t __bootloader;
extern uint32_t __firmware;