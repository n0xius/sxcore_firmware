#include "spi.h"

#include "gw_defines.h"
#include "delay.h"
#include "rgb_led.h"

extern uint32_t __bootloader;
extern uint32_t __firmware;

extern int memcmp ( const void* _Ptr1, const void* _Ptr2, uint32_t _Size );
extern void* memcpy ( void* _Dst, const void* _Src, uint32_t _Size );
extern void* memset ( void* _Dst, int _Val, uint32_t _Size );

uint8_t fpga_is_cdone_set(void)
{
	return gpio_input_bit_get(GPIOA, GPIO_PIN_1) != RESET;
}

void fpga_spi0_reset_nss(void)
{
	gpio_bit_reset(GPIOA, GPIO_PIN_4);
}

void fpga_spi0_reset_creset(void)
{
	gpio_bit_reset(GPIOB, GPIO_PIN_10);
}

void spi1_set_cs_low(void)
{
	gpio_bit_reset(GPIOB, GPIO_PIN_12);
}

void fpga_spi0_wait_and_set_nss(void)
{
	while ( spi_i2s_flag_get(SPI0, SPI_STAT_TRANS) != RESET );

	gpio_bit_set(GPIOA, GPIO_PIN_4);
}

void fpga_spi0_set_creset(void)
{
	gpio_bit_set(GPIOB, GPIO_PIN_10);
}

void spi1_set_cs_high(void)
{
	gpio_bit_set(GPIOB, GPIO_PIN_12);
}

void initialize_spi0(uint32_t _spi_prescale)
{
	spi_parameter_struct spi_parameter;

	spi_parameter.frame_size = SPI_FRAMESIZE_8BIT;
	spi_parameter.device_mode = SPI_MASTER;
	spi_parameter.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
	spi_parameter.nss = SPI_NSS_SOFT;
	spi_parameter.endian = SPI_ENDIAN_MSB;
	spi_parameter.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
	spi_parameter.prescale = _spi_prescale;

	spi_i2s_deinit(SPI0);
	spi_init(SPI0, &spi_parameter);
	spi_ti_mode_disable(SPI0);
	spi_enable(SPI0);
}

void initialize_spi1()
{
	rcu_periph_clock_enable(RCU_SPI1);

	// LISC3H Pin 8 CS, SPI Idle/Communication
	gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_12);
	gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12);

	// LISC3H Pin 4 SCL, SPI CLK
	gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_13);
	gpio_af_set(GPIOB, GPIO_AF_0, GPIO_PIN_13);

	// LISC3H Pin 7 SDO, SPI SDO
	gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_14);
	gpio_af_set(GPIOB, GPIO_AF_0, GPIO_PIN_14);

	// LISC3H Pin 6 SDI, SPI SDI
	gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_15);
	gpio_af_set(GPIOB, GPIO_AF_0, GPIO_PIN_15);

	spi_parameter_struct spi_param;

	spi_param.device_mode = SPI_MASTER;
	spi_param.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
	spi_param.nss = SPI_NSS_SOFT;
	spi_param.endian = SPI_ENDIAN_MSB;
	spi_param.prescale = SPI_PSC_32;
	spi_param.frame_size = SPI_FRAMESIZE_8BIT;
	spi_param.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;

	spi_init(SPI1, &spi_param);
	spi_ti_mode_disable(SPI1);
	spi_i2s_interrupt_enable(SPI1, SPI_I2S_INT_TBE);
	spi_i2s_interrupt_enable(SPI1, SPI_I2S_INT_RBNE);
	spi_enable(SPI1);

	spi1_set_cs_high();
}

void spi0_transmit_data(uint8_t *_data, uint32_t _data_len)
{
	for(uint32_t i = 0; i < _data_len; ++i)
	{
		spi_i2s_data_transmit(SPI0, (uint32_t)_data[i]);

		// wait until TBE/RBNE is set
		while ( (SPI_STAT(SPI0) & (SPI_STAT_TBE|SPI_STAT_RBNE)) != (SPI_STAT_RBNE | SPI_STAT_TBE) );

		_data[i] = (uint8_t)(spi_i2s_data_receive(SPI0) & 0xFF);
	}

	// wait until spi transaction is done.
	while ( ((SPI_STAT(SPI0) & 0xFF) & (SPI_STAT_TRANS|SPI_STAT_TBE|SPI_STAT_RBNE)) != SPI_STAT_TBE );
}

void spi0_send_data(uint8_t *_data, uint32_t _data_len)
{
	for(uint32_t i = 0; i < _data_len; ++i)
	{
		spi_i2s_data_transmit(SPI0, (uint32_t)_data[i]);

		// wait until TBE/RBNE is set
		while ( (SPI_STAT(SPI0) & (SPI_STAT_TBE | SPI_STAT_RBNE)) != (SPI_STAT_RBNE | SPI_STAT_TBE) );
	}

	// wait until spi transaction is done.
	while ( ((SPI_STAT(SPI0) & 0xFF) & (SPI_STAT_TRANS|SPI_STAT_TBE|SPI_STAT_RBNE)) != SPI_STAT_TBE );
}

void spi1_recv_data(uint8_t _cmd, uint8_t *_data, uint32_t _data_len)
{
	spi1_set_cs_low();

	spi_i2s_data_transmit(SPI1, (uint32_t)_cmd);

	// wait until TBE/RBNE is set
	while ( (SPI_STAT(SPI1) & (SPI_STAT_TBE | SPI_STAT_RBNE)) != (SPI_STAT_RBNE | SPI_STAT_TBE) );

	for(uint32_t i = 0; i < _data_len; ++i)
	{
		spi_i2s_data_transmit(SPI1, (uint32_t)0xFF);

		// wait until TBE/RBNE is set
		while ( (SPI_STAT(SPI1) & (SPI_STAT_TBE | SPI_STAT_RBNE)) != (SPI_STAT_RBNE | SPI_STAT_TBE) );

		_data[i] = (uint8_t)(spi_i2s_data_receive(SPI1) & 0xFF);
	}

	// wait until spi transaction is done.
	while ( ((SPI_STAT(SPI1) & 0xFF) & (SPI_STAT_TRANS | SPI_STAT_TBE | SPI_STAT_RBNE)) != SPI_STAT_TBE );

	spi1_set_cs_high();
}

void spi1_send_data(uint8_t _cmd, uint8_t *_data, uint32_t _data_len)
{
	spi1_set_cs_low();

	spi_i2s_data_transmit(SPI1, (uint32_t)_cmd);

	// wait until TBE/RBNE is set
	while ( (SPI_STAT(SPI1) & (SPI_STAT_TBE|SPI_STAT_RBNE)) != (SPI_STAT_RBNE | SPI_STAT_TBE) );

	for(uint32_t i = 0; i < _data_len; ++i)
	{
		spi_i2s_data_transmit(SPI1, (uint32_t)_data[i]);

		// wait until TBE/RBNE is set
		while ( (SPI_STAT(SPI1) & (SPI_STAT_TBE|SPI_STAT_RBNE)) != (SPI_STAT_RBNE | SPI_STAT_TBE) );
	}

	// wait until spi transaction is done.
	while ( ((SPI_STAT(SPI1) & 0xFF) & (SPI_STAT_TRANS | SPI_STAT_TBE | SPI_STAT_RBNE)) != SPI_STAT_TBE );

	spi1_set_cs_high();
}

uint8_t spi0_transfer_one_byte(uint8_t _data)
{
	uint8_t buffer = _data;

	spi0_transmit_data(&buffer, 1);

	return (uint8_t)(buffer & 0xFF);
}

void spi0_send_one_byte(uint8_t _data)
{
	uint8_t buffer = _data;
	spi0_send_data(&buffer, 1);
}

void spi0_send_clk(uint32_t _size)
{
	for(uint32_t i = 0; i < _size; ++i)
		spi0_send_one_byte(0);
}

// https://github.com/YosysHQ/icestorm/blob/master/iceprog/iceprog.c#L232
uint32_t spi0_read_status()
{
	spi0_send_clk(8);

	for (int i = 0; i <= 9000; ++i)
	{
		fpga_spi0_reset_nss();

		// fpga read status register
		uint8_t send_buffer[2] = { 5, 0 };
		spi0_transmit_data(send_buffer, 2);

		fpga_spi0_wait_and_set_nss();

		uint8_t flags = send_buffer[1];

		if ( (flags & 1) != 0 )
			continue;
		
		spi0_send_clk(8);

		if ( !(flags & 0xFC) )
			return GW_STATUS_SUCCESS;

		if ( (flags & 0x10) != 0 )
			return 0xBAD0000F;

		if ( !(flags & 0xE0) )
			return 0xBAD00011;

		return 0xBAD00010;
	}

	return 0xBAD0000B;
}

//void spi0_send_data_24(uint8_t _cmd, uint8_t _data_len, uint32_t _data)
void spi0_send_data_24(uint32_t _cmd_and_size, uint32_t _data)
{
	uint8_t buffer[16];

	//int cmd_len = ((uint8_t*)(&_cmd_and_size))[1];
	int cmd_len = (uint8_t)(_cmd_and_size >> 8);

	if (cmd_len >= 4)
		cmd_len = 4;

	buffer[0] = 0x24;
	buffer[1] = (uint8_t)(_cmd_and_size);

	for (int i = 0; i != cmd_len; ++i)
	{
		buffer[i + 2] = _data;
		_data >>= 8;
	}

	fpga_spi0_reset_nss();
	spi0_send_data(buffer, cmd_len + 2);

	fpga_spi0_wait_and_set_nss();
}

//uint32_t spi0_recv_data_26(uint8_t _cmd, uint8_t _data_len)
uint32_t spi0_recv_data_26(uint32_t _cmd_and_size)
{
	uint8_t buffer[16];

	//int cmd_len = ((uint8_t*)(&_cmd_and_size))[1];
	int cmd_len = (uint8_t)(_cmd_and_size >> 8);

	if (cmd_len >= 4)
		cmd_len = 4;

	buffer[0] = 0x26;
	buffer[1] = (uint8_t)(_cmd_and_size);

	fpga_spi0_reset_nss();
	spi0_transmit_data(buffer, cmd_len + 2);
	fpga_spi0_wait_and_set_nss();

	uint32_t data = 0;

	for (int i = 0; i != cmd_len; ++i)
	{
		//data <<= 8;
		//data |= buffer[i + 2];
		data = (data << 8) | buffer[i + 2];
	}

	return data;
}

// fpga read active buffer data
void spi0_recv_data_BA(uint8_t* _data, uint32_t _data_len)
{
	fpga_spi0_reset_nss();

	spi0_send_one_byte(0xBA);
	spi0_transmit_data(_data, _data_len);

	fpga_spi0_wait_and_set_nss();
}

// fpga write active buffer data
void spi0_send_data_BC(uint8_t* _data, uint32_t _data_len)
{
	fpga_spi0_reset_nss();

	spi0_send_one_byte(0xBC);
	spi0_send_data(_data, _data_len);

	fpga_spi0_wait_and_set_nss();
}

// fpga reset write enable latch (WRDI)
void spi0_send_4(void)
{
	fpga_spi0_reset_nss();

	// program disable
	spi0_send_one_byte(4); // fpga reset write enable latch (WRDI)

	fpga_spi0_wait_and_set_nss();

	spi0_send_clk(8);
}

// fpga select active buffer
void spi0_send_05_via_24(uint8_t _data)
{
	spi0_send_data_24(0x105, _data);
}

// fpga set write enable latch (WREN) and read status
uint32_t spi0_get_fpga_cmd(void)
{
	fpga_spi0_reset_nss();

	// program enable
	spi0_send_one_byte(6); // fpga set write enable latch (WREN)

	fpga_spi0_wait_and_set_nss();

	spi0_send_clk(8);

	return spi0_read_status();
}

void spi0_send_fpga_cmd(uint8_t _data)
{
	spi0_send_data_24(0x106, _data);
}

void spi0_send_07_via_24(uint8_t _data)
{
	spi0_send_data_24(0x107, _data);
}

// fpga read status flags
uint32_t spi0_recv_0B_via_26(void)
{
	return spi0_recv_data_26(0x10B);
}

void spi0_send_05_recv_BA(uint8_t _buffer_index, uint8_t* _data, uint32_t _data_len)
{
	spi0_send_05_via_24(_buffer_index);
	spi0_recv_data_BA(_data, _data_len);
}

void spi0_send_05_send_BC(uint8_t _buffer_index, uint8_t* _data, uint32_t _data_len)
{
	spi0_send_05_via_24(_buffer_index);
	spi0_send_data_BC(_data, _data_len);
}

// source for the iCE40 fpga information: https://github.com/osresearch/icestorm/blob/nvcmtool/icenvcm/icenvcm.py
void fpga_nvcm_read(uint32_t _address, uint8_t *_data)
{
	fpga_spi0_reset_nss();

	uint8_t buffer[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	buffer[0] = 0x03; // fpga read data
	buffer[1] = (uint8_t)(_address >> 16);
	buffer[2] = (uint8_t)(_address >> 8);
	buffer[3] = (uint8_t)(_address);

	spi0_send_data(buffer, sizeof(buffer));

	memset(_data, 0, 8);

	spi0_transmit_data(_data, 8);
	fpga_spi0_wait_and_set_nss();
	spi0_send_clk(8);
}

// fpga nvcm trim write
uint32_t spi0_send_82_and_quad_word(uint8_t *_data)
{
	fpga_spi0_reset_nss();

	// NOTE: 0x20 = 8 bits x word size (2)
	uint8_t buffer[4] = { 0x82, 0, 0, 0x20 };
	spi0_send_data(buffer, sizeof(buffer));

	spi0_send_data(_data, 8);

	fpga_spi0_wait_and_set_nss();

	return spi0_read_status();
}

// fpga nvcm bank select
uint32_t fpga_select_bank(uint8_t _data)
{
	fpga_spi0_reset_nss();

	uint8_t buffer[5] = { 0x83, 0, 0, 0x25, _data };
	spi0_send_data(buffer, sizeof(buffer));

	fpga_spi0_wait_and_set_nss();

	return spi0_read_status();
}

// fpga nvcm access enable
void spi0_send_0_C4_via_82()
{
	uint8_t buffer[8] = { 0, 0, 0, 0, 0xC4, 0, 0, 0 };

	spi0_send_82_and_quad_word(buffer);
}

// fpga nvcm write
uint32_t spi0_write_11_bytes_via_02(uint8_t *_data)
{
	fpga_spi0_reset_nss();

	spi0_send_one_byte(2);
	spi0_send_data(_data, 11);

	fpga_spi0_wait_and_set_nss();

	spi0_send_clk(16);

	return spi0_read_status();
}

// fpga nvcm trim program
uint32_t spi0_send_11_bytes_0_15_f2_f1_c4(uint8_t _data)
{
	uint8_t buffer[11] = { 0, 0, _data, 0, 0x15, 0xF2, 0xF1, 0xC4, 0, 0, 0 };
	return spi0_write_11_bytes_via_02(buffer);
}

// fpga nvcm trim secure
uint32_t spi0_send_11_bytes_30_0_0_1_0(uint8_t _data)
{
	uint8_t buffer[11] = { 0, 0, _data, 0x30, 0, 0, 1, 0, 0, 0, 0 };
	return spi0_write_11_bytes_via_02(buffer);
}

// fpga nvcm trim enable
uint32_t spi0_send_8_bytes_via_82(void)
{
	uint8_t buffer[8] = { 0, 0x15, 0xF2, 0xF0, 0xC2, 0, 0, 0, };
	return spi0_send_82_and_quad_word(buffer);
}

uint32_t spi0_setup(uint32_t _prescale_select)
{
	initialize_spi0(_prescale_select == 2 ? SPI_PSC_8 : SPI_PSC_4);

	fpga_spi0_reset_creset();

	if ( (_prescale_select & 0xFFFFFFFD) != 0)
	{
		// fpga configure as spi periphal
		if ( _prescale_select == 1 )
			fpga_spi0_wait_and_set_nss();
	}
	else // fpga configure from NVCM
		fpga_spi0_reset_nss();

	delay_us(300);

	fpga_spi0_set_creset();

	if ( (_prescale_select & 0xFFFFFFFD) != 0)
	{
		if ( _prescale_select == 1 )
		{
			delay_ms(50);

			// if CDONE == 0 in SPI periphal mode, the fpga failed to start correctly
			if ( !fpga_is_cdone_set() )
				return GW_STATUS_FPGA_STARTUP_FAILED; // 0xBAD00004
		}

		return GW_STATUS_SUCCESS; // 0x900D0000
	}

	delay_us(10);

	if ( fpga_is_cdone_set() )
		return GW_STATUS_FPGA_STARTUP_FAILED; // 0xBAD00004

	delay_ms(1);

	if ( _prescale_select != 2 )
		return GW_STATUS_SUCCESS; // 0x900D0000

	initialize_spi0(SPI_PSC_8);

	// fpga "nvcm unlock access" sequence
	uint8_t buffer[8] = { 0x7E, 0xAA, 0x99, 0x7E, 0x01, 0x0E, 0, 0 };

	// send "nvcm unlock access" sequence to fpga
	spi0_send_data(buffer, 6);

	fpga_spi0_wait_and_set_nss();

	spi0_send_clk(5000);

	uint32_t status = spi0_read_status();

	// check if fpga is able to communitcate?
	if ( status != GW_STATUS_SUCCESS )
		return status;
	
	// select "silicon signature" bank
	fpga_select_bank(0x20); 

	fpga_nvcm_read(0, buffer); // read silicon signature

	if ( buffer[0] != 8 )
		return GW_STATUS_FPGA_SILICON_SIG_MISMATCH; // 0xBAD0000E
	
	// re-select "nvcm" bank
	fpga_select_bank(0); 
	spi0_send_0_C4_via_82();
	fpga_nvcm_read(0, buffer);

	return GW_STATUS_SUCCESS;
}

uint32_t spi0_init_with_psc_4(void)
{
	return spi0_setup(1);
}