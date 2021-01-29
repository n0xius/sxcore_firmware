#pragma once

#include <gd32f3x0.h>

uint32_t mmc_send_command(uint8_t cmd, uint32_t argument, uint8_t *response, uint8_t *_data);
uint32_t write_bct_and_payload(uint8_t *_cid, uint8_t _device_type);
uint32_t reset_bct_and_erase_payload();