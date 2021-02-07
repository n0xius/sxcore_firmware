#pragma once

#include <gd32f3x0.h>
#include <stdarg.h>
#include <stdint.h>

/* this function handles timer13 interrupts */
void timer13_irq_handler(void);

/* this function handles usbfs interrupts */
void usbfs_irq_handler(void);

extern int memcmp ( const void* _Ptr1, const void* _Ptr2, uint32_t _Size );
extern void* memcpy ( void* _Dst, const void* _Src, uint32_t _Size );
extern void* memset ( void* _Dst, int _Val, uint32_t _Size );

extern void aes128_cipher(const uint8_t *_key, uint8_t *_data);

extern int _vsnprintf(char* buffer, const uint32_t maxlen, const char* format, va_list va);

typedef void (*irq_handler_t) (void);

extern const uint32_t* firmware_version;

// defined in linker.ld
extern const uint32_t* __bootloader;
extern const uint32_t* __firmware;
extern const uint32_t* __config;
