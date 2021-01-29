#pragma once

#include <gd32f3x0.h>

uint32_t get_firmware_buffer_position(void);
uint32_t get_firmware_block_checksum(void);

uint32_t validate_firmware_header(uint8_t* _firmware_buffer, uint32_t _should_erase);
uint32_t handle_firmware_update(uint8_t* _firmware_buffer);
