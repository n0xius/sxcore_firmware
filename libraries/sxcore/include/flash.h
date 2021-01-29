#pragma once

#include <gd32f3x0.h>

uint32_t flash_erase(uint32_t _page_address);
uint32_t flash_reprogram(uint8_t *_Dst, uint8_t *_Src, uint32_t _Size);