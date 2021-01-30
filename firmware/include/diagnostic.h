#pragma once

#include <gd32f3x0.h>
#include "gw_defines.h"

//extern diagnostic_print_s g_diagnosis_print;
extern const diagnostic_print_s g_no_diagnosis_print;

void handle_usb_diagnostic(bootloader_usb_s* _usb);

void diagnosis_begin(const diagnostic_print_s *_diag_print);
void diagnosis_hexdump(const diagnostic_print_s *_diag_print, uint8_t _type, uint8_t *_data, uint32_t _size);
void diagnosis_end(const diagnostic_print_s *_diag_print);

void diagnosis_hexdump_spi_start(const diagnostic_print_s *_diag_print);
void diagnosis_hexdump_fpga(const diagnostic_print_s *_diag_print, fpga_config_s *_fpga_cfg, uint8_t _spi_status, uint32_t _data_size, uint8_t *_data, uint8_t a6, uint8_t a7);
void diagnosis_hexdump_config(const diagnostic_print_s *_diag_print, config_s *_cfg);
void diagnosis_hexdump_fpga_cfg(const diagnostic_print_s *_diag_print, fpga_config_s *_config, uint32_t _status);
void diagnosis_hexdump_mmc_cid(const diagnostic_print_s *_diag_print, uint32_t _result, uint8_t *_cid);
void diagnosis_hexdump_device_type(const diagnostic_print_s *_diag_print, uint8_t _device_type);
void diagnosis_hexdump_fw_version(const diagnostic_print_s *_diag_print, uint32_t _version);
void diagnosis_hexdump_bldr_version(const diagnostic_print_s *_diag_print, uint32_t _version);
void diagnosis_hexdump_adc(const diagnostic_print_s *_diag_print, uint32_t _adc);
void diagnosis_hexdump_serial(const diagnostic_print_s *_diag_print, uint8_t *_serial);