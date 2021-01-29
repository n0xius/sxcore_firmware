/*!
    \file  main.c
    \brief CRC calculate demo

    \version 2017-06-06, V1.0.0, firmware for GD32F3x0
    \version 2019-06-01, V2.0.0, firmware for GD32F3x0
*/

/*
    Copyright (c) 2019, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "gd32f3x0.h"
#include "systick.h"
#include <stdio.h>
#include "gd32f350r_eval.h"

uint32_t vab1 = 0, success_flag = 0;
uint32_t read32_1, read32_2, read32_3, read32_4, read32_5, read32_6, read32_7, read32_8;

/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    /* enable the LED1 clock */
    rcu_periph_clock_enable(RCU_GPIOC);
    /* configure LED1 GPIO port */ 
    gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_10);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    gpio_bit_reset(GPIOC, GPIO_PIN_10);

    vab1 = (uint32_t)0xabcd1234;
    rcu_periph_clock_enable(RCU_CRC);
    
    crc_deinit();
    read32_1 = crc_single_data_calculate(vab1);

    /* input reverse */
    crc_deinit();
    crc_input_data_reverse_config(CRC_INPUT_DATA_BYTE);
    read32_2 = crc_single_data_calculate(vab1);

    crc_deinit();
    crc_input_data_reverse_config(CRC_INPUT_DATA_HALFWORD);
    read32_3 = crc_single_data_calculate(vab1);

    crc_deinit();
    crc_input_data_reverse_config(CRC_INPUT_DATA_WORD);
    read32_4 = crc_single_data_calculate(vab1);

    /* output reverse */
    crc_deinit();
    crc_reverse_output_data_enable();
    read32_5 = crc_single_data_calculate(vab1);

    crc_deinit();
    crc_input_data_reverse_config(CRC_INPUT_DATA_BYTE);
    crc_reverse_output_data_enable();
    read32_6 = crc_single_data_calculate(vab1);

    crc_deinit();
    crc_input_data_reverse_config(CRC_INPUT_DATA_HALFWORD);
    crc_reverse_output_data_enable();
    read32_7 = crc_single_data_calculate(vab1);

    crc_deinit();
    crc_input_data_reverse_config(CRC_INPUT_DATA_WORD);
    crc_reverse_output_data_enable();
    read32_8 = crc_single_data_calculate(vab1);

    /* check the caculation result */
    if((read32_1 == 0xf7018a40U)&&(read32_2 == 0x49fc6721U)&&(read32_3 == 0x606444e3U)&&(read32_4 == 0x16d70081U)
        &&(read32_5 == 0x025180efU)&&(read32_6 == 0x84e63f92U)&&(read32_7 == 0xc7222606U)&&(read32_8 == 0x8100eb68U)){
        success_flag = 0x1U;
        gpio_bit_set(GPIOC, GPIO_PIN_10);
    }

    while (1);
}
