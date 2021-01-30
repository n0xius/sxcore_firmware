#pragma once

#include <gd32f3x0.h>
#include <stdarg.h>
#include <stdint.h>

/* this function handles device resets */
void reset_handler(void);

/* this function handles interrupts of timer13 */
void timer13_irq_handler(void);

extern int memcmp ( const void* _Ptr1, const void* _Ptr2, uint32_t _Size );
extern void* memcpy ( void* _Dst, const void* _Src, uint32_t _Size );
extern void* memset ( void* _Dst, int _Val, uint32_t _Size );

extern void aes128_cipher(const uint8_t *_key, uint8_t *_data);

extern int _vsnprintf(char* buffer, const uint32_t maxlen, const char* format, va_list va);

// defined in linker.ld
extern uint32_t __bootloader;
extern uint32_t __firmware;
extern uint32_t __config;

extern uint32_t __data_start__;
extern uint32_t __data_size__;
extern uint32_t __bss_start__;
extern uint32_t __bss_size__;