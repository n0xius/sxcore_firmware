#include "gw_defines.h"
#include "emmc.h"
#include "spi.h"
#include "bct.h"
#include "delay.h"

extern int memcmp ( const void* _Ptr1, const void* _Ptr2, uint32_t _Size );
extern void* memcpy ( void* _Dst, const void* _Src, uint32_t _Size );
extern void* memset ( void* _Dst, int _Val, uint32_t _Size );

uint8_t crc7(uint8_t *_buffer, uint32_t _size)
{
    uint32_t crc = 0;

    for(int i = 0; i < _size; ++i)
    {
        uint32_t msg_byte = _buffer[i];

        for(int j = 0; j < 8; ++j)
        {
            crc <<= 1;

            if ( ((crc ^ msg_byte) & 0x80) != 0 ) // check if negative
                crc ^= 9;

            msg_byte <<= 1;
        }

        crc &= 0x7F; // make positive
    }

    return (uint8_t)crc;
}

void spi0_send_value_via_05_24(uint8_t _value)
{
    spi0_send_data_24(0x05, 1, _value);
}

void mmc_spi_response_get(uint8_t *response)
{
    spi0_send_value_via_05_24(0);
    spi0_recv_data_BA(response, 32);
}

void spi0_send_54()
{
    fpga_spi0_reset_nss();

    uint8_t cmd = 0x54;
    spi0_send_data(&cmd, 1u);

    fpga_spi0_wait_and_set_nss();
}

uint32_t mmc_send_command(uint8_t cmd, uint32_t argument, uint8_t *response, uint8_t *_data)
{
    uint8_t cmd_buffer[7];

    cmd_buffer[0] = cmd | 0x40;
    cmd_buffer[1] = argument >> 24;
    cmd_buffer[2] = argument >> 16;
    cmd_buffer[3] = argument >> 8;
    cmd_buffer[4] = argument;
    cmd_buffer[5] = (crc7(cmd_buffer, 5) << 1) | 1;
    cmd_buffer[6] = 1;

    switch ( cmd )
    {
        case 2:                                 // MMC_ALL_SEND_CID
        case 9:                                 // MMC_SEND_CSD
        {
            cmd_buffer[6] |= 8u;
            break;
        }

        case 17:                                // MMC_READ_SINGLE_BLOCK
        {
            cmd_buffer[6] = 3;
            break;
        }

        case 24:                                // MMC_WRITE_BLOCK
        {
            if ( _data )
            {
                spi0_send_value_via_05_24(1);
                spi0_send_data_BC(_data, 512);
            }

            cmd_buffer[6] = 5;
            break;
        }
    }

    spi0_send_value_via_05_24(0);
    spi0_send_data_BC(cmd_buffer, 7);
    spi0_send_54();

    int timeout = 2000;

    while ( (spi0_recv_0B_via_26() & 1) != 0 )
    {
        if ( !--timeout )
            return -1;

        delay_us(50);
    }

    mmc_spi_response_get(response);

    if ( cmd == 17 && _data ) // MMC_READ_SINGLE_BLOCK
    {
        spi0_send_value_via_05_24(1);
        spi0_recv_data_BA(_data, 512);
    }

    return 0;
}

uint32_t mmc_spi_check_response(uint8_t* _response, uint32_t _value)
{
    uint32_t data = __builtin_bswap32((uint32_t)&_response[1]);
    return ((_value | 0x100) - (data | 0x100)) != 0 ? 0xFFFFFFFF : 0;
}

void fpga_reset_and_enable_mmc(uint8_t _enable_emmc)
{
    spi0_send_fpga_cmd(0x80);
    delay_ms(2);
    spi0_send_fpga_cmd(0);

    if ( _enable_emmc != 1 )
        return;

    delay_ms(15);
    spi0_send_fpga_cmd(0x40);
    delay_ms(2000);
    spi0_send_fpga_cmd(0);
    delay_ms(1);
}

uint32_t mmc_initialize(uint8_t *_cid)
{
    uint8_t emmc_response[32];

    fpga_reset_and_enable_mmc(1);

    // MMC_GO_IDLE_STATE
    if ( mmc_send_command(0, 0, emmc_response, 0) )                     // reset mmc device
        return GW_STATUS_MMC_GO_IDLE_STATE_FAILED;                      // 0xBAD0010D

    // MMC_SEND_OP_COND
    if ( mmc_send_command(1, 0, emmc_response, 0) )                     // initialize mmc device
        return GW_STATUS_MMC_SEND_OP_COND_FAILED;                       // 0xBAD0010E

    int retry = 100;

    // MMC_SEND_OP_COND
    while ( !mmc_send_command(1, 0x40000080u, emmc_response, 0) )       // initialize mmc speed
    {
        if ( emmc_response[1] == 0xC0 )                                 // EXT_CSD_CARD_TYPE_HS400?
        {
            // MMC_ALL_SEND_CID
            if ( mmc_send_command(2, 0, emmc_response, 0) )             // retrieve card id
                return GW_STATUS_MMC_ALL_SEND_CID_FAILED;               // 0xBAD00111

            if ( _cid )
            {
                memcpy(_cid, (uint8_t*)emmc_response + 1, 16);
                _cid[15] = crc7(_cid, 15) | 1;
            }

            // MMC_SET_RELATIVE_ADDR
            if ( mmc_send_command(3, 0x00020000, emmc_response, 0) )   // check chip size by setting 0x40000 (512 x 512)?
                return GW_STATUS_MMC_SET_RELATIVE_ADDR_FAILED;          // 0xBAD00112

            if ( mmc_spi_check_response(emmc_response, 0x500) )
                return GW_STATUS_MMC_SET_RELATIVE_ADDR_RESPONSE;        // 0xBAD00113

            // MMC_SEND_CSD
            if ( mmc_send_command(0, 0x00020000, emmc_response, 0) )      // get csd data
                return GW_STATUS_MMC_SEND_CSD_FAILED;                   // 0xBAD00114

            // MMC_SELECT_CARD
            if ( mmc_send_command(7, 0x00020000, emmc_response, 0) )
                return GW_STATUS_MMC_SELECT_CARD_FAILED;                // 0xBAD00115

            if ( mmc_spi_check_response(emmc_response, 0x700) )
                return GW_STATUS_MMC_SELECT_CARD_RESPONSE;              // 0xBAD00116

            // MMC_SEND_STATUS
            if ( mmc_send_command(13, 0x00020000, emmc_response, 0) )     // check if card is alive
                return GW_STATUS_MMC_SEND_STATUS_FAILED;                // 0xBAD00117

            if ( mmc_spi_check_response(emmc_response, 0x900) )
                return GW_STATUS_MMC_SEND_STATUS_RESPONSE;              // 0xBAD00118

            // MMC_SET_BLOCKLEN
            if ( mmc_send_command(16, 512, emmc_response, 0) )         // set block size
                return GW_STATUS_MMC_SET_BLOCKLEN_FAILED;               // 0xBAD00119

            if ( mmc_spi_check_response(emmc_response, 0x900) )
                return GW_STATUS_MMC_SET_BLOCKLEN_RESPONSE;             // 0xBAD0011A

            // MMC_SWITCH
            if ( mmc_send_command(6, 0x03B30100, emmc_response, 0) )    // EMMC_SWITCH_SELECT_PARTITION_ARG
                return GW_STATUS_MMC_SWITCH_FAILED;                     // 0xBAD0011B

            if ( mmc_spi_check_response(emmc_response, 0x900) )
                return GW_STATUS_MMC_SWITCH_RESPONSE;                   // 0xBAD0011C

            return 0;
        }

        delay_ms(10);

        if ( !--retry )
            break;
    }

    return GW_STATUS_MMC_SEND_OP_COND_TIMEOUT; // 0xBAD00110
}

uint32_t mmc_read(uint32_t _block_index, uint8_t *_buffer)
{
    uint8_t emmc_response[32];

    // MMC_READ_SINGLE_BLOCK
    if ( mmc_send_command(17, _block_index, emmc_response, _buffer) )
        return GW_STATUS_MMC_READ_SINGLE_BLOCK_FAILED;          // 0xBAD0011D

    if ( mmc_spi_check_response(emmc_response, 0x900) )
        return GW_STATUS_MMC_READ_SINGLE_BLOCK_RESPONSE;        // 0xBAD0011E

    return 0;
}

uint32_t mmc_copy(uint32_t _dst, uint32_t _src, uint32_t _size)
{
    uint8_t emmc_response[32];
    uint8_t buffer[512];

    memset(buffer, 0, sizeof(buffer));

    for ( uint32_t i = 0; i != _size; ++i )
    {
        // MMC_READ_SINGLE_BLOCK
        if ( mmc_send_command(17, i + _src, emmc_response, buffer) )
            return GW_STATUS_MMC_READ_SINGLE_BLOCK_FAILED;      // 0xBAD0011D

        if ( mmc_spi_check_response(emmc_response, 0x900) )
            return GW_STATUS_MMC_READ_SINGLE_BLOCK_RESPONSE;    // 0xBAD0011E

        // MMC_WRITE_BLOCK
        if ( mmc_send_command(24, i + _dst, emmc_response, buffer) )
            return GW_STATUS_MMC_WRITE_BLOCK_FAILED;            // 0xBAD00120

        if ( mmc_spi_check_response(emmc_response, 0x900) )
            return GW_STATUS_MMC_WRITE_BLOCK_RESPONSE;          // 0xBAD00121
    }

    return 0;
}

uint32_t mmc_erase(uint32_t _dst, uint32_t _size)
{
    uint8_t emmc_response[32];
    uint8_t buffer[512];

    memset(buffer, 0, sizeof(buffer));

    uint32_t block_size = (_size + 511) >> 9;

    for ( uint32_t i = 0; i != block_size; ++i )
    {
        // MMC_WRITE_BLOCK
        if ( mmc_send_command(24, i + _dst, emmc_response, buffer) )
            return GW_STATUS_MMC_WRITE_BLOCK_FAILED;            // 0xBAD00120

        if ( mmc_spi_check_response(emmc_response, 0x900) )
            return GW_STATUS_MMC_WRITE_BLOCK_RESPONSE;          // 0xBAD00121
    }

    return 0;
}

uint32_t mmc_compare_and_overwrite_if_not_equal(uint32_t _block_index, const uint8_t *_data, uint32_t _size)
{
    uint8_t emmc_response[32];
    uint8_t buffer[512];

    memset(buffer, 0, sizeof(buffer));

    uint32_t block_size = (_size + 511) >> 9;

    for ( uint32_t i = 0; i != block_size; ++i )
    {
        // MMC_READ_SINGLE_BLOCK
        if ( mmc_send_command(17, _block_index + i, emmc_response, buffer) )
            return GW_STATUS_MMC_READ_SINGLE_BLOCK_FAILED;      // 0xBAD0011D

        if ( mmc_spi_check_response(emmc_response, 0x900) )
            return GW_STATUS_MMC_READ_SINGLE_BLOCK_RESPONSE;    // 0xBAD0011E

        int safe_block_size = _size >= 512 ? 512 : _size;

        if ( memcmp(buffer, _data, safe_block_size) != 0 )
        {
            memcpy(buffer, _data, safe_block_size);

            // MMC_WRITE_BLOCK
            if ( mmc_send_command(24, _block_index + i, emmc_response, buffer) )
                return GW_STATUS_MMC_WRITE_BLOCK_FAILED;            // 0xBAD00120

            if ( mmc_spi_check_response(emmc_response, 0x900) )
                return GW_STATUS_MMC_WRITE_BLOCK_RESPONSE;          // 0xBAD00121
        }

        _size -= safe_block_size;
        _data += safe_block_size;
    }

    return 0;
}

uint32_t write_bct_and_payload(uint8_t *_cid, uint8_t _device_type)
{
    uint8_t read_buffer[512];

    const uint8_t *bct_data = _device_type == DEVICE_TYPE_ERISTA ? erista_bct : mariko_bct;
    uint32_t bct_size = _device_type == DEVICE_TYPE_ERISTA ? sizeof(erista_bct) : sizeof(mariko_bct);

    uint32_t status = 0xBAD0010C;

    for(int i = 0; i < 6; ++i)
    {
        status = mmc_initialize(_cid);

        if ( status != 0 )
            continue;

        // 64 * 512 = 0x8000
        status = mmc_read(64, read_buffer);

        if ( status != 0 )
            continue;

        // 0 * 512 = 0
        status = mmc_compare_and_overwrite_if_not_equal(0, bct_data, bct_size);
        if ( status != 0 )
            continue;

        // 32 * 512 = 0x4000
        status = mmc_compare_and_overwrite_if_not_equal(32, bct_data, bct_size);
        if ( status != 0 )
            continue;

        // 0x1F88 * 512 = 0x3F1000
        status = mmc_compare_and_overwrite_if_not_equal(0x1F88, stage1_payload, sizeof(stage1_payload));
        if ( status != 0 )
            continue;

        // 0x1F80 * 512 = 0x3F0000
        status = mmc_compare_and_overwrite_if_not_equal(0x1F80, stage0_payload, sizeof(stage0_payload));
        if ( status != 0 )
            continue;

        status = GW_STATUS_BCT_PAYLOAD_SUCCESS; // 0x900D0008
        break;
    }

    return status;
}

uint32_t reset_bct_and_erase_payload()
{
    uint32_t status = 0xBAD0010C;

    for(int i = 0; i < 6; ++i)
    {
        status = mmc_initialize(0);
        if ( status != 0 )
            continue;

        status = mmc_copy(0, 64, 20);
        if ( status != 0 )
            continue;

        status = mmc_copy(0x20, 96, 20);
        if ( status != 0 )
            continue;

        status = mmc_erase(0x1F88, 0x4000);
        if ( status != 0 )
            continue;

        status = mmc_erase(0x1F80, 0x1000);
        if ( status != 0 )
            continue;

        status = 0x900D0008;
        break;
    }

    return status;
}