#include "gd32f3x0_it.h"

#include "gw_defines.h"
#include "configuration.h"
#include "diagnostic.h"
#include "rgb_led.h"
#include "glitch.h"
#include "delay.h"
#include "emmc.h"
#include "spi.h"

const uint16_t erista_glitch_offsets[17] = {
    825, 830, 835, 840, 845, 850, 855, 860, 865, 870, 875, 880, 885, 890, 895, 900, 905u
};

const uint16_t mariko_glitch_offsets[13] = {
    820, 825, 830, 835, 840, 845, 850, 855, 860, 865, 870, 875, 880u
};

static uint32_t g_random_seed = 0x75BCD15;

uint32_t get_random_number()
{
    g_random_seed = 1664525 * g_random_seed + 0x3C6EF35F;
    return g_random_seed;
}

void setup_adc_for_gpio_pin(uint32_t _gpio, uint32_t _gpio_pin, uint8_t _adc_channel)
{
    adc_deinit();
    rcu_adc_clock_config(RCU_ADCCK_APB2_DIV6);
    rcu_periph_clock_enable(RCU_ADC);
    gpio_mode_set(_gpio, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, _gpio_pin);
    adc_channel_length_config(ADC_REGULAR_CHANNEL, 1);
    adc_regular_channel_config(0, _adc_channel, ADC_SAMPLETIME_1POINT5);
    adc_external_trigger_config(ADC_REGULAR_CHANNEL, DISABLE);
    adc_external_trigger_source_config(ADC_REGULAR_CHANNEL, ADC_EXTTRIG_REGULAR_NONE);
    adc_data_alignment_config(ADC_DATAALIGN_RIGHT);
    adc_resolution_config(ADC_RESOLUTION_12B);
    adc_special_function_config(ADC_CONTINUOUS_MODE, DISABLE);
    adc_special_function_config(ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(ADC_INSERTED_CHANNEL_AUTO, DISABLE);
    adc_software_trigger_enable(ADC_REGULAR_CHANNEL);
    adc_enable();
    adc_calibration_enable();
}

uint32_t adc_channel_read(void)
{
    //adc_special_function_config(ADC_CONTINUOUS_MODE, ENABLE);
	adc_enable();

    while ( adc_flag_get(ADC_FLAG_EOC) == RESET );

    return (uint32_t)adc_regular_data_read();
}

uint32_t get_pin_by_board_type(void)
{
    FlagStatus pin_3 = gpio_input_bit_get(GPIOB, GPIO_PIN_3);
    FlagStatus pin_5 = gpio_input_bit_get(GPIOB, GPIO_PIN_5);

    if ( pin_3 == RESET && pin_5 == RESET ) // sx core
        return GPIO_PIN_0;

    if ( pin_3 == SET && pin_5 == RESET )   // sx lite
        return GPIO_PIN_1;

    return 0;
}

uint32_t get_device_type(void)
{
    spi0_send_fpga_cmd(0x80u);
    delay_ms(1);
    spi0_send_fpga_cmd(0);
    delay_ms(1);

    uint32_t pinId = get_pin_by_board_type();

    if ( pinId == GPIO_PIN_0 )
    {
        setup_adc_for_gpio_pin(GPIOB, GPIO_PIN_0, ADC_CHANNEL_8);
        uint32_t erista_adc = adc_channel_read();
        gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_0);

        setup_adc_for_gpio_pin(GPIOB, GPIO_PIN_1, ADC_CHANNEL_9);
        uint32_t mariko_adc = adc_channel_read();
        gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_1);

        if ( mariko_adc > 255 )
            return mariko_adc != 256 ? DEVICE_TYPE_UNKNOWN : DEVICE_TYPE_MARIKO;

        return erista_adc > 255 ? DEVICE_TYPE_ERISTA : DEVICE_TYPE_UNKNOWN;
    }

    if ( pinId == GPIO_PIN_1 ) // sx lite
    {
        setup_adc_for_gpio_pin(GPIOA, GPIO_PIN_2, ADC_CHANNEL_2);
        uint32_t lite_adc = adc_channel_read();
        // NOTE: shouldn't be PA2 be reset as well when both above get reset after adc usage?
        //gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_2);

        return lite_adc > 255 ? DEVICE_TYPE_LITE : DEVICE_TYPE_UNKNOWN;
    }

    return DEVICE_TYPE_UNKNOWN;
}

uint32_t setup_for_board_type(device_type _device_type, uint16_t _adc_data[2])
{
    uint32_t status = GW_STATUS_NO_OR_UNKNOWN_DEVICE; // 0xBAD00107

    switch(_device_type)
    {
        case DEVICE_TYPE_ERISTA:
        {
            setup_adc_for_gpio_pin(GPIOB, GPIO_PIN_0, ADC_CHANNEL_8);

            _adc_data[0] = 1200;
            _adc_data[1] = 1376;

            status = 0;
            break;
        }

        case DEVICE_TYPE_MARIKO:
        case DEVICE_TYPE_LITE:
        {
            if (_device_type == DEVICE_TYPE_MARIKO)
                setup_adc_for_gpio_pin(GPIOB, GPIO_PIN_1, ADC_CHANNEL_9);
            else
                setup_adc_for_gpio_pin(GPIOA, GPIO_PIN_2, ADC_CHANNEL_2);

            _adc_data[0] = 1024;
            _adc_data[1] = 1296;

            status = 0;
            break;
        }

        default:
            break;
    }

    return status;
}

uint32_t reset_fpga_and_read_adc_value(const diagnostic_print_s *_diag_print, uint32_t _adc_threshold, uint16_t *_adc_out)
{
    spi0_send_fpga_cmd(0x80);
    delay_ms(1);
    spi0_send_fpga_cmd(0);

    uint32_t adc_value = adc_channel_read();
    diagnosis_hexdump_adc(_diag_print, adc_value | 0x30000000);

    int i = 0;
    while ( 1 )
    {
        adc_value = adc_channel_read();

        if ( _adc_out )
            *_adc_out = adc_value;

        if ( adc_value >= _adc_threshold )
        {
            diagnosis_hexdump_adc(_diag_print, adc_value | 0x10000000);
            return 0;
        }

        ++i;
        delay_us(500);

        if ( i == 2000 )
            break;

        if ( (i << 26) == 0 )
            diagnosis_hexdump_adc(_diag_print, adc_value);
    }

    diagnosis_hexdump_adc(_diag_print, adc_value | 0x20000000);
    return GW_STATUS_ADC_CHANNEL_READ_MISMATCH; // 0xBAD00122
}

uint32_t glitch_and_get_device_type(uint32_t *_device_type)
{
    uint16_t adc_threshold[2];
    uint32_t device_type = get_device_type();

    if (_device_type)
        *_device_type = device_type;

    uint32_t status = setup_for_board_type(device_type, adc_threshold);

    if ( !status )
    {
        status = reset_fpga_and_read_adc_value(0, adc_threshold[0], 0);

        if ( !status )
            return GW_STATUS_SUCCESS;
    }

    return status;
}

uint32_t glitch_and_boot(void)
{
    uint16_t adc_threshold[2];
    uint32_t device = get_device_type();

    spi0_send_fpga_cmd(0x80);
    delay_ms(200);
    spi0_send_fpga_cmd(0);

    uint32_t status = setup_for_board_type(device, adc_threshold);

    if ( !status )
    {
        status = reset_fpga_and_read_adc_value(0, adc_threshold[0], 0);

        if ( !status )
            return GW_STATUS_SUCCESS;
    }

    return status;
}

uint8_t did_toggle_chip()
{
    spi0_send_fpga_cmd(4);
    spi0_send_fpga_cmd(1);

    for ( uint32_t timeout = 0; timeout <= 7000; ++timeout )
    {
        if ( (spi0_recv_0B_via_26() & 0x10) != 0 )
        {
            // NOTE: 0x43 = toggle chip state
            if ( execute_spi_command() == 0x43 )
                return 1;

            timeout = 0;
        }

        delay_us(50);
    }

    return 0;
}

void spi_parser_init(spi_parser_s *_spi_parser, uint8_t *_buffer, uint32_t _size)
{
    _spi_parser->buffer = _buffer;
    _spi_parser->buffer_length = _size;
    _spi_parser->datatype = 0;
    _spi_parser->cmd = 0;
}

uint32_t spi_parser_parse(spi_parser_s *_spi_parser)
{
    if ( _spi_parser->buffer_length <= 5 )
        return 3;

    uint8_t* buffer = _spi_parser->buffer;
    uint8_t flags = buffer[0];

    uint32_t args = __builtin_bswap32((uint32_t)(buffer + 1));

    if ( (flags & 0x40) == 0 )
    {
        if ( _spi_parser->buffer_length <= 16 )
            return 3;

        if ( _spi_parser->cmd == 2 || _spi_parser->cmd == 9 )
        {
            _spi_parser->orig_buffer = buffer;
            _spi_parser->datatype = 2;
            _spi_parser->cmd = 0;
        }
        else
        {
            _spi_parser->datatype = 1;
            _spi_parser->cmd = flags & 0x3F;
            _spi_parser->args = args;
        }
    }
    else
    {
        _spi_parser->datatype = 0;
        _spi_parser->cmd = flags & 0x3F;
        _spi_parser->args = args;
    }

    if ( _spi_parser->datatype == 2 )
    {
        _spi_parser->buffer += 17;
        _spi_parser->buffer_length -= 17;
    }
    else
    {
        _spi_parser->buffer += 6;
        _spi_parser->buffer_length -= 6;
    }

    return _spi_parser->datatype;
}

uint32_t spi0_get_data_with_size(uint8_t *_buffer)
{
    uint32_t flags = spi0_recv_0B_via_26() & 0x40;

    if ( flags )
    {
        spi0_send_05_recv_BA(0, _buffer, 512);
        flags = (uint32_t)_buffer[16];
        spi0_send_05_recv_BA(2, _buffer, 512);
    }

    return flags;
}

uint32_t run_glitch(const diagnostic_print_s *_diag_print, fpga_config_s *_fpga_cfg)
{
    config_s cfg;
    fpga_config_s fpga_config;
    spi_parser_s spi_parser;
    uint8_t spi_data[512];

    uint32_t was_config_loaded = _fpga_cfg ? 3 : 2;

    if ( _fpga_cfg )
        config_clear(&cfg);

    int fpga_cfg_tries = !_fpga_cfg ? config_load_from_flash(&cfg) == GW_STATUS_CONFIG_INVALID_MAGIC : 0;

    uint32_t device = get_device_type();
    uint32_t glitch_offsets_size = device == DEVICE_TYPE_ERISTA ? 17 : 13;
    uint16_t *glitch_offsets = (uint16_t*)(device == DEVICE_TYPE_ERISTA ? erista_glitch_offsets : mariko_glitch_offsets);

    uint16_t adc_threshold = 0;
    uint16_t adc_threshold_ranges[2] = { 0, 0 };
    uint32_t status = setup_for_board_type(device, adc_threshold_ranges);

    diagnosis_begin(_diag_print);

    uint8_t* g_serial_number = ((uint8_t*)__bootloader + 0x150);
    uint32_t* g_bootloader_version = (uint32_t*)((uint8_t*)__bootloader + 0x160);

    diagnosis_hexdump_serial(_diag_print, g_serial_number);
    diagnosis_hexdump_device_type(_diag_print, device);
    diagnosis_hexdump_fw_version(_diag_print, *firmware_version);
    diagnosis_hexdump_bldr_version(_diag_print, *g_bootloader_version);

    toggle_blue_led_glow(1);

    if ( was_config_loaded == 2 )
        diagnosis_hexdump_config(_diag_print, &cfg);

    int max_width_changes = device == DEVICE_TYPE_ERISTA ? 3 : 5;

    uint8_t did_reset_fpga = 0;
    if ( !status )
    {
        while ( 1 )
        {
            uint16_t adc_value = 0;
            status = reset_fpga_and_read_adc_value(_diag_print, adc_threshold_ranges[1], &adc_value);

            if ( adc_threshold_ranges[1] <= adc_value )
            {
                adc_threshold = adc_threshold_ranges[1];
                break;
            }

            if ( adc_value >= adc_threshold_ranges[0] )
            {
                adc_threshold = adc_threshold_ranges[0];
                status = 0;
                break;
            }

            if ( !did_reset_fpga )
            {
                spi0_send_fpga_cmd(0x80u);
                delay_ms(500);
                spi0_send_fpga_cmd(0);
                did_reset_fpga = 1;
            }
        }
    }

    int32_t width = device == DEVICE_TYPE_ERISTA ? 35 : 53;
    uint32_t offset = device == DEVICE_TYPE_ERISTA ? 876 : 1210;

    uint32_t saved_fpga_data = 0;

    for ( int i = 0; i != cfg.saved_glitch_data; ++i )
    {
        width = cfg.width[i];

        if (!saved_fpga_data && i >= 4)
            saved_fpga_data = 4;
    }

    fpga_config.offset = offset;
    fpga_config.rng = 0;
    fpga_config.width = width;

    if ( !status )
    {
        diagnosis_hexdump_spi_start(_diag_print);

        uint8_t spi_cmds[128];
        memset(spi_cmds, 0, sizeof(spi_cmds));

        uint8_t spi_cmd_offset = 0;
        int prev_fpga_cfg_tries = 0;
        int cur_glitch_offset_idx = 0;
        int cycles = 0;

        while ( 1 )
        {
            prev_fpga_cfg_tries = fpga_cfg_tries;
            fpga_cfg_tries = 0;

            ++cycles;

            if ( was_config_loaded == 3 )
            {
                if ( cycles > 49 )
                {
                    status = GW_STATUS_GLITCH_TIMEOUT; // 0xBAD00124
                    break;
                }
            }
            else if ( was_config_loaded == 2 )
            {
                if ( cycles >= 1200 )
                {
                    status = GW_STATUS_GLITCH_TIMEOUT; // 0xBAD00124
                    break;
                }

                if ( cycles >= 400 )
                {
                    cycles = 0;
                    saved_fpga_data = 0;
                    prev_fpga_cfg_tries = 1;
                }
            }

            if ( adc_channel_read() < adc_threshold )
            {
                status = reset_fpga_and_read_adc_value(_diag_print, adc_threshold, 0);

                if ( status )
                    break;
            }

            if ( prev_fpga_cfg_tries )
            {
                uint8_t mmc_cid[16];
                status = write_bct_and_payload(mmc_cid, device);
                diagnosis_hexdump_mmc_cid(_diag_print, status, mmc_cid);

                if ( status != GW_STATUS_BCT_PAYLOAD_SUCCESS )
                    break;
            }

            if ( was_config_loaded == 3 )
            {
                fpga_config.width = _fpga_cfg->width;
                fpga_config.offset = _fpga_cfg->offset;

                int32_t rng1 = _fpga_cfg->rng;

                if ( rng1 < 0 )
                    rng1 = 0;
                else if ( rng1 > 255 )
                    rng1 = 255;

                fpga_config.rng = rng1;
            }
            else
            {
                if ( saved_fpga_data > fpga_cfg_tries && cfg.saved_glitch_data )
                {
                    uint32_t v26 = get_random_number();
                    fpga_config.offset = cfg.offsets[v26 - v26 / cfg.saved_glitch_data * cfg.saved_glitch_data];
                    ++fpga_cfg_tries;
                }
                else
                {
                    ++cur_glitch_offset_idx;

                    if ( cur_glitch_offset_idx > glitch_offsets_size )
                        cur_glitch_offset_idx = 0;

                    fpga_config.offset = glitch_offsets[cur_glitch_offset_idx];
                    fpga_cfg_tries = 0;
                }

                fpga_config.rng = (uint8_t)(get_random_number() & 3);
            }

            int32_t clampedWidth = fpga_config.width;

            if ( clampedWidth < 2 )
                clampedWidth = 2;

            if ( clampedWidth > 200 )
                clampedWidth = 200;

            fpga_config.width = clampedWidth;

            spi0_send_fpga_cmd(0);

            spi0_send_data_24(0x03, 1, 120);
            spi0_send_data_24(0x01, 2, fpga_config.offset);
            spi0_send_data_24(0x08, 1, fpga_config.rng);
            spi0_send_data_24(0x02, 1, fpga_config.width);

            spi0_send_fpga_cmd(0x80);
            delay_ms(1);
            spi0_send_fpga_cmd(0x10);

            uint32_t spi_status = 0;
            while ( 1 )
            {
                spi_status = spi0_recv_0B_via_26();

                if ( (spi_status & 2) != 0 || (spi_status & 4) != 0 )
                    break;

                spi0_recv_data_26(0x0A, 1);
            }

            uint8_t v43 = spi0_recv_data_26(0x0A, 1);
            uint32_t spi_data_len = spi0_get_data_with_size(spi_data);

            int spi_data_type;

            if (spi_data_len >= 5)
                spi_data_type = 1;
            else
                spi_data_type = 3;

            spi_parser_init(&spi_parser, spi_data, spi_data_len);

            while ( 1 )
            {
                uint32_t data_type = spi_parser_parse(&spi_parser);

                if ( data_type == 3 )
                    break;

                if (data_type == 0)
                {
                    if ( spi_parser.cmd == 17 || spi_parser.cmd == 0 )
                        spi_data_type = 2;
                }
                else
                {
                    if ( data_type == 1 && spi_data_type == 3 )
                        spi_data_type = 1;
                }
            }

            uint8_t toggled_chip = (spi_status & 2) ? did_toggle_chip() : 0;

            diagnosis_hexdump_fpga(_diag_print, &fpga_config, spi_status, spi_data_len, spi_data, v43, toggled_chip);

            spi_cmds[spi_cmd_offset++] = spi_data_type;

            spi_cmd_offset &= 7;

            int total_spi_commands = 0;
            int total_type_1_spi_cmd = 0;
            int total_type_2_spi_cmd = 0;
            int total_type_3_spi_cmd = 0;

            for ( int j = 0; j != 8; ++j )
            {
                if ( !spi_cmds[j] )
                    continue;

                ++total_spi_commands;

                switch ( spi_cmds[j] )
                {
                    case 1:
                        ++total_type_1_spi_cmd;
                        break;
                    case 2:
                        ++total_type_2_spi_cmd;
                        break;
                    case 3:
                        ++total_type_3_spi_cmd;
                        break;
                }
            }

            if ( total_spi_commands == 8 )
            {
                if ( total_type_2_spi_cmd > 4 )
                {
                    fpga_config.width += 1;
                    memset(spi_cmds, 0, sizeof(spi_cmds));
                }

                if ( max_width_changes <= total_type_1_spi_cmd )
                {
                    fpga_config.width -= 1;
                    memset(spi_cmds, 0, sizeof(spi_cmds));
                }

                if ( total_type_3_spi_cmd == 8 )
                    memset(spi_cmds, 0, sizeof(spi_cmds));
            }

            if ( total_type_3_spi_cmd == 8 )
            {
                status = 0xBAD00108;
                break;
            }

            if ( toggled_chip )
            {
                if ( was_config_loaded == 3 )
                {
                    status = GW_STATUS_GLITCH_SUCCESS; // 0x900D0006
                    break;
                }

                if ( was_config_loaded == 2 )
                {
                    if ( config_save_fpga_cfg(&cfg, &fpga_config) == GW_STATUS_CONFIG_SUCCESS )
                    {
                        uint32_t save_status = config_write_to_flash(&cfg);
                        diagnosis_hexdump_fpga_cfg(_diag_print, &fpga_config, save_status);
                    }

                    status = GW_STATUS_GLITCH_SUCCESS; // 0x900D0006
                    break;
                }
            }
        }
    }

    diagnosis_end(_diag_print);

    toggle_blue_led_glow(0);

    uint32_t led_color;

    switch ( status )
    {
        case GW_STATUS_GLITCH_SUCCESS:              // 0x900D0006
            led_color = LED_COLOR_GREEN;
            break;
        case GW_STATUS_GLITCH_FAILED:               // 0xBAD00108
            led_color = 0x00FFFFFF;
            break;
        case GW_STATUS_ADC_CHANNEL_READ_MISMATCH:   // 0xBAD00122
            led_color = LED_COLOR_PINK;
            break;
        default:
            led_color = LED_COLOR_RED;
            break;
    }

    set_led_color(led_color);
    return status;
}