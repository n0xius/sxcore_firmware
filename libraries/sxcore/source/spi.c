#include "spi.h"
#include "delay.h"
#include "rgb_led.h"
#include "gw_defines.h"

extern uint32_t __bootloader;
extern uint32_t __firmware;

extern int memcmp ( const void* _Ptr1, const void* _Ptr2, uint32_t _Size );
extern void* memcpy ( void* _Dst, const void* _Src, uint32_t _Size );
extern void* memset ( void* _Dst, int _Val, uint32_t _Size );

uint8_t fpga_is_cdone_set(void)
{
    return gpio_input_bit_get(GPIOA, GPIO_PIN_1) != RESET;
}

// since this is configured as pull up, this will set the pin to high
void fpga_spi0_reset_nss(void)
{
    gpio_bit_reset(GPIOA, GPIO_PIN_4);
}

// since this is configured as pull up, this will set the pin to high
void fpga_spi0_reset_creset(void)
{
    gpio_bit_reset(GPIOB, GPIO_PIN_10);
}

void fpga_spi1_reset_creset(void)
{
    gpio_bit_reset(GPIOB, GPIO_PIN_12);
}

void fpga_spi0_wait_and_set_nss(void)
{
    while (spi_i2s_flag_get(SPI0, SPI_STAT_TRANS) != RESET);

    gpio_bit_set(GPIOA, GPIO_PIN_4);
}

// since this is configured as pull up, this will set the pin to low
void fpga_spi0_set_creset(void)
{
    gpio_bit_set(GPIOB, GPIO_PIN_10);
}

// since this is configured as pull up, this will set the pin to low
void fpga_spi1_set_nss(void)
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

    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_12);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12);

    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_13);
    gpio_af_set(GPIOB, GPIO_AF_0, GPIO_PIN_13);

    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_14);
    gpio_af_set(GPIOB, GPIO_AF_0, GPIO_PIN_14);

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

    fpga_spi1_set_nss();
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
    fpga_spi1_reset_creset();

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

    fpga_spi1_set_nss();
}

void spi1_send_data(uint8_t _cmd, uint8_t *_data, uint32_t _data_len)
{
    fpga_spi1_reset_creset();

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

    fpga_spi1_set_nss();
}

uint8_t spi0_transfer_one_byte(uint8_t _data)
{
    uint8_t buffer = _data;

    spi0_transmit_data(&buffer, 1);

    return (uint8_t)(buffer & 0xFF);
}

void spi0_send_clk(uint32_t _size)
{
    uint8_t buffer = 0;

    for(uint32_t i = 0; i < _size; ++i)
        spi0_send_data(&buffer, 1);
}

int spi0_read_status()
{
    uint8_t v3[2] = { 5, 0 };

    spi0_send_clk(8);

    uint32_t i = 9001;

    do
    {
        fpga_spi0_reset_nss();

        spi0_transmit_data(v3, 2);

        fpga_spi0_wait_and_set_nss();

        if ( !(v3[1] & 1) )
            break;

        if (--i == 0)
            return 0xBAD0000B;
    } while(i != 0);

    spi0_send_clk(8);

    if ( (v3[1] & 0xFC) == 0 )
        return GW_STATUS_SUCCESS; // 0x900D0000

    if ( (v3[1] & 0x10) != 0 )
        return 0xBAD0000F;

    if ( (v3[1] & 0xE0) == 0 )
        return 0xBAD00011;

    return 0xBAD00010;
}

void spi0_send_data_24(uint8_t _cmd, uint8_t _data_len, uint32_t _data)
{
    fpga_spi0_reset_nss();

    spi0_transfer_one_byte(0x24);
    spi0_transfer_one_byte(_cmd);

    for ( int i = 0; i != _data_len; ++i )
    {
        spi0_transfer_one_byte((uint8_t)(_data & 0xFF));
        _data >>= 8;
    }

    fpga_spi0_wait_and_set_nss();
}

uint32_t spi0_recv_data_26(uint8_t _cmd, uint8_t _data_len)
{
    fpga_spi0_reset_nss();

    spi0_transfer_one_byte(0x26);
    spi0_transfer_one_byte(_cmd);

    uint32_t data = 0;

    for(int i = 0; i != _data_len; ++i)
    {
        data <<= 8;
        data |= spi0_transfer_one_byte(0);
    }

    fpga_spi0_wait_and_set_nss();
    return data;
}

void spi0_recv_data_BA(uint8_t* _data, uint32_t _data_len)
{
    fpga_spi0_reset_nss();

    spi0_transfer_one_byte(0xBA);

    for(int i = 0; i != _data_len; ++i)
        _data[i] = spi0_transfer_one_byte(0);

    fpga_spi0_wait_and_set_nss();
}

void spi0_send_data_BC(uint8_t* _data, uint32_t _data_len)
{
    fpga_spi0_reset_nss();

    spi0_transfer_one_byte(0xBC);

    for(int i = 0; i != _data_len; ++i)
        spi0_transfer_one_byte(_data[i]);

    fpga_spi0_wait_and_set_nss();
}

void spi0_send_4(void)
{
    fpga_spi0_reset_nss();

    uint8_t buffer = 4;
    spi0_send_data(&buffer, 1);

    fpga_spi0_wait_and_set_nss();

    spi0_send_clk(8u);
}

void spi0_send_05_via_24(uint8_t _data)
{
    spi0_send_data_24(0x05, 1, _data);
}

uint32_t spi0_get_fpga_cmd(void)
{
    fpga_spi0_reset_nss();

    uint8_t buffer = 6;
    spi0_send_data(&buffer, 1);

    fpga_spi0_wait_and_set_nss();

    spi0_send_clk(8);

    return spi0_read_status();
}

void spi0_send_fpga_cmd(uint8_t _data)
{
    spi0_send_data_24(0x06, 1, _data);
}

void spi0_send_07_via_24(uint8_t _data)
{
    spi0_send_data_24(0x07, 1, _data);
}

uint32_t spi0_recv_0B_via_26(void)
{
    return spi0_recv_data_26(0x0B, 1);
}

void spi0_send_05_recv_BA(uint8_t _cmd, uint8_t* _data, uint32_t _data_len)
{
    spi0_send_05_via_24(_cmd);
    spi0_recv_data_BA(_data, _data_len);
}

void spi0_send_05_send_BC(uint8_t _cmd, uint8_t* _data, uint32_t _data_len)
{
    spi0_send_05_via_24(_cmd);
    spi0_send_data_BC(_data, _data_len);
}

void spi0_send_03_read_8_bytes(uint32_t a1, uint8_t *a2)
{
    fpga_spi0_reset_nss();

    uint8_t v4[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    v4[0] = 3;
    v4[1] = (uint32_t)(a1 >> 16) & 0xFF;
    v4[2] = (uint32_t)(a1 >> 8) & 0xFF;
    v4[3] = (uint32_t)(a1) & 0xFF;

    spi0_send_data(v4, sizeof(v4));

    a2[0] = 0;
    a2[1] = 0;
    a2[2] = 0;
    a2[3] = 0;
    a2[4] = 0;
    a2[5] = 0;
    a2[6] = 0;
    a2[7] = 0;

    spi0_transmit_data(a2, 8);
    fpga_spi0_wait_and_set_nss();
    spi0_send_clk(8);
}

uint32_t spi0_send_82_and_quad_word(uint8_t *a1)
{
    fpga_spi0_reset_nss();

    // NOTE: 0x20 = 8 bits x word size (2)
    uint8_t v3[4] = { 0x82, 0, 0, 0x20 };
    spi0_send_data(v3, 4);

    spi0_send_data(a1, 8);

    fpga_spi0_wait_and_set_nss();

    return spi0_read_status();
}

uint32_t spi0_transfer_83(uint8_t a1)
{
    fpga_spi0_reset_nss();

    uint8_t v3[5] = { 0x83, 0, 0, 0x25, a1 };
    spi0_transmit_data(v3, 5);

    fpga_spi0_wait_and_set_nss();

    return spi0_read_status();
}

uint8_t spi_cmd_0_0_C4[8] = { 0, 0, 0, 0, 0xC4, 0, 0, 0 };

void spi0_send_0_C4_via_82()
{
    uint8_t v1[8];

    for(uint32_t i = 0; i < sizeof(v1); ++i)
        v1[i] = spi_cmd_0_0_C4[i];

    spi0_send_82_and_quad_word(v1);    // 0 0 0 0 c4 0 0 0
}

uint32_t spi0_write_11_bytes_via_02(uint8_t *_data)
{
    fpga_spi0_reset_nss();

    uint8_t buffer = 2;

    spi0_send_data(&buffer, 1);
    spi0_send_data(_data, 11);

    fpga_spi0_wait_and_set_nss();

    spi0_send_clk(16u);

    return spi0_read_status();
}

uint32_t spi0_send_11_bytes_0_15_f2_f1_c4(uint8_t _data)
{
    uint8_t buffer[11] = { 0, 0, _data, 0, 0x15, 0xF2, 0xF1, 0xC4, 0, 0, 0 };
    return spi0_write_11_bytes_via_02(buffer);
}

uint32_t spi0_send_11_bytes_30_0_0_1_0(uint8_t _data)
{
    uint8_t buffer[11] = { 0, 0, _data, 0x30, 0, 0, 1, 0, 0, 0, 0 };
    return spi0_write_11_bytes_via_02(buffer);
}

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
        if ( _prescale_select == 1 )
            fpga_spi0_wait_and_set_nss();
    }
    else
        fpga_spi0_reset_nss();

    delay_us(300);

    fpga_spi0_set_creset();

    if ( (_prescale_select & 0xFFFFFFFD) != 0)
    {
        if ( _prescale_select == 1 )
        {
            delay_ms(50);

            if ( !fpga_is_cdone_set() )
                return 0xBAD00004;
        }

        return GW_STATUS_SUCCESS; // 0x900D0000
    }

    delay_us(10);

    if ( fpga_is_cdone_set() )
        return 0xBAD00004;

    delay_ms(1);

    if ( _prescale_select != 2 )
        return GW_STATUS_SUCCESS; // 0x900D0000

    initialize_spi0(SPI_PSC_8);

    // some synchronization word for the fpga
    uint8_t v5[8] = { 0x7E, 0xAA, 0x99, 0x7E, 0x01, 0x0E, 0, 0 };

    spi0_send_data(v5, 6);

    fpga_spi0_wait_and_set_nss();

    spi0_send_clk(5000);

    uint32_t status = spi0_read_status();

    // check if emmc is able to communitcate?
    if ( status != GW_STATUS_SUCCESS )
        return status;

    // read the 8 bytes at 0x4000 (32 x 512)?
    spi0_transfer_83(32);

    spi0_send_03_read_8_bytes(0, v5);

    if ( v5[0] != 8 )
        return 0xBAD0000E;

    spi0_transfer_83(0);
    spi0_send_0_C4_via_82();
    spi0_send_03_read_8_bytes(0, v5);

    return GW_STATUS_SUCCESS;
}

uint32_t spi0_init_with_psc_4(void)
{
    return spi0_setup(1);
}