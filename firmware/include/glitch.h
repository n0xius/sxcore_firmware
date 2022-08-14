#pragma once

#include <gd32f3x0.h>

void setup_adc_for_gpio_pin(uint32_t _gpio, uint32_t _gpio_pin, uint8_t _adc_channel);

uint16_t adc_channel_read(void);

uint32_t get_pin_by_board_type(void);

uint32_t get_device_type(void);

uint32_t glitch_and_get_device_type(uint32_t *_device_type);
uint32_t glitch_and_boot(void);

uint32_t run_glitch(const diagnostic_print_s *_diag_print, fpga_config_s *_fpga_cfg);

extern uint8_t execute_spi_command();