#pragma once

#include <gd32f3x0.h>

/* this function handles device resets */
void reset_handler(void);
/* this function handles USBFS IRQ Handler */
void usbfs_irq_handler(void);

extern int memcmp ( const void* _Ptr1, const void* _Ptr2, uint32_t _Size );
extern void* memcpy ( void* _Dst, const void* _Src, uint32_t _Size );
extern void* memset ( void* _Dst, int _Val, uint32_t _Size );

typedef void (*irq_handler_t) (void);

extern const uint8_t* serial_number;
extern const uint32_t* debug_mode;
extern const uint32_t* bootloader_version;

// defined in linker.ld
extern const uint32_t* __bootloader;
extern const uint32_t* __firmware;
extern const uint32_t* __config;
