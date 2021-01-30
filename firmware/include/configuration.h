#pragma once

#include <gd32f3x0.h>

extern config_s* g_saved_config;
extern fpga_config_s g_fpga_config;

void config_clear(config_s *_config);

uint8_t config_is_chip_enabled(config_s *_config);
void config_set_chip_enabled(config_s *_config, uint8_t _chip_enabled);

uint32_t config_save_fpga_cfg(config_s *_config, fpga_config_s *_fpga_cfg);

uint32_t config_load_from_flash(config_s *_config);
uint32_t config_write_to_flash(config_s *_config);

uint8_t is_chip_disabled(void);