#pragma once

#include <gd32f3x0.h>

void gpioa_turn_off_pin4(void);
void gpiob_turn_off_pin10(void);
void gpioa_spi0_wait_and_turn_on_pin4(void);
void gpiob_turn_on_pin10(void);

void initialize_spi0(uint32_t _spi_prescale);
void initialize_spi1(void);

void spi0_transmit_data(uint8_t *_data, uint32_t _data_len);
void spi0_send_data(uint8_t *_data, uint32_t _data_len);

void spi1_recv_data(uint8_t _cmd, uint8_t *_data, uint32_t _data_len);
void spi1_send_data(uint8_t _cmd, uint8_t *_data, uint32_t _data_len);

uint8_t spi0_transfer_one_byte(uint8_t _data);

void spi0_send_clk(uint32_t _size);
int spi0_read_status();

void spi0_send_data_24(uint8_t _cmd, uint8_t _data_len, uint32_t _data);
uint32_t spi0_recv_data_26(uint8_t _cmd, uint8_t _data_len);

void spi0_recv_data_BA(uint8_t* _data, uint32_t _data_len);
void spi0_send_data_BC(uint8_t* _data, uint32_t _data_len);

uint32_t spi0_write_11_bytes_via_02(uint8_t* _data);

uint32_t spi0_send_11_bytes_0_15_f2_f1_c4(uint8_t _data);
uint32_t spi0_send_11_bytes_30_0_0_1_0(uint8_t _data);

uint32_t spi0_send_8_bytes_via_82(void);

void spi0_send_4(void);
void spi0_send_05_via_24(uint8_t _data);
uint32_t spi0_get_fpga_cmd(void);
void spi0_send_fpga_cmd(uint8_t _data);
void spi0_send_07_via_24(uint8_t _data);
uint32_t spi0_recv_0B_via_26(void);

void spi0_send_05_recv_BA(uint8_t _cmd, uint8_t* _data, uint32_t _data_len);
void spi0_send_05_send_BC(uint8_t _cmd, uint8_t* _data, uint32_t _data_len);

void spi0_send_03_read_8_bytes(uint32_t a1, uint8_t *a2);

uint32_t spi0_send_82_and_quad_word(uint8_t *a1);
uint32_t spi0_transfer_83(uint8_t a1);
void spi0_send_0_C4_via_82();

uint32_t spi0_setup(uint32_t a1);

uint32_t spi0_init_with_psc_4(void);