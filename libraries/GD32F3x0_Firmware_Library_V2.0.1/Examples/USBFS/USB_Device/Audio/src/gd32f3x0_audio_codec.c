/*!
    \file  gd32f3x0_audio_codec.c
    \brief this file contains all the audio codec low layer driver

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

#include "gd32f3x0_audio_codec.h"

/* Mask for the bit EN of the I2S CFGR register */
#define I2S_ENABLE_MASK                 (0x0400)

/* delay for the Codec to be correctly reset */
#define CODEC_RESET_DELAY               (0x4FFF)

/* Codec audio Standards */
#ifdef I2S_STANDARD_PHILLIPS
    #define CODEC_STANDARD                 0x04
    #define I2S_STANDARD                   I2S_STD_PHILLIPS
#elif defined(I2S_STANDARD_MSB)
    #define CODEC_STANDARD                 0x00
    #define I2S_STANDARD                   I2S_STD_MSB
#elif defined(I2S_STANDARD_LSB)
    #define CODEC_STANDARD                 0x08
    #define I2S_STANDARD                   I2S_STD_LSB
#else 
    #error "Error: No audio communication standard selected !"
#endif /* I2S_STANDARD */

/* The 7 bits Codec adress (sent through I2C interface) */
#define CODEC_ADDRESS                      0x94  /* b00100111 */

/* This structure is declared global because it is handled by two different functions */
static dma_parameter_struct dma_initstructure;
static uint8_t outputdev = 0;

uint32_t audiototalsize = 0xFFFF; /* This variable holds the total size of the audio file */
uint32_t audioremsize   = 0xFFFF; /* This variable holds the remaining data in audio file */
uint16_t *currentpos;             /* This variable holds the current position of audio pointer */
uint32_t i2s_audiofreq = 0;

/* Audio Codec functions */

/* Low layer codec functions */
static void     codec_ctrlinterface_init    (void);
static void     codec_ctrlinterface_deinit  (void);
static void     codec_audiointerface_init   (uint32_t audiofreq);
static void     codec_audiointerface_deinit (void);
static void     codec_reset                 (void);
static uint32_t codec_writeregister         (uint32_t registeraddr, uint32_t registervalue);
static void     codec_gpio_init             (void);
static void     codec_gpio_deinit           (void);
static void     delay                       (__IO uint32_t ncount);

/*!
    \brief      configure the audio peripherals
    \param[in]  outputdevice:
                    @arg OUTPUT_DEVICE_SPEAKER
                    @arg OUTPUT_DEVICE_HEADPHONE
                    @arg OUTPUT_DEVICE_BOTH
                    @arg OUTPUT_DEVICE_AUTO	              
    \param[in]  volume: Initial volume level (from 0 (Mute) to 100 (Max))
	\param[in]  audiofreq: Audio frequency used to play the audio stream
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t eval_audio_init(uint16_t outputdevice, uint8_t volume, uint32_t audiofreq)
{
    /* Perform low layer Codec initialization */
    if (codec_init(outputdevice, VOLUME_CONVERT(volume), audiofreq) != 0) {
        return 1;
    } else {
        /* I2S data transfer preparation:
           Prepare the Media to be used for the audio transfer from memory to I2S peripheral */
        audio_mal_init();

        /* Return 0 when all operations are OK */
        return 0;
    }
}

/*!
    \brief      deinitializes all the resources used by the codec (those initialized 
                by eval_audio_init() function) EXCEPT the I2C resources since they are 
                used by the IOExpander as well (and eventually other modules).           
    \param[in]  none
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t eval_audio_deinit(void)
{
    /* deinitialize the media layer */
    audio_mal_deinit();

    /* deinitialize codec */
    codec_deinit();

    return 0;
}

/*!
    \brief      configure the audio peripherals
    \param[in]  pbuffer: Pointer to the buffer             
    \param[in]  size: number of audio data bytes
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t eval_audio_play(uint16_t* pbuffer, uint32_t size)
{
    /* Set the total number of data to be played (count in half-word) */
    audiototalsize = size/2;

    /* Call the audio Codec Play function */
    codec_play();

    /* Update the Media layer and enable it for play */  
    audio_mal_play((uint32_t)pbuffer, (uint32_t)(DMA_MAX(audiototalsize / 2)));

    /* Update the remaining number of data to be played */
    audioremsize = (size/2) - DMA_MAX(audiototalsize);

    /* Update the current audio pointer position */
    currentpos = pbuffer + DMA_MAX(audiototalsize);

    return 0;
}

/*!
    \brief      this function pauses or resumes the audio file stream. in case
                of using DMA, the DMA pause feature is used. in all cases the i2s 
                peripheral is disabled
    \param[in]  cmd: AUDIO_PAUSE (or 0) to pause, AUDIO_RESUME (or any value different from 0) to resume
    \param[in]  addr: Address from/at which the audio stream should resume/pause	
    \param[in]  size: number of data to be configured for next resume
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t eval_audio_pauseresume(uint32_t cmd, uint32_t addr, uint32_t size)
{
    if (cmd != AUDIO_PAUSE) {
        /* Call the Media layer pause/resume function */
        audio_mal_pauseresume(cmd, addr, size);

        /* Call the Audio Codec Pause/Resume function */
        if (codec_pauseresume(cmd) != 0) {
            return 1;
        } else {
            return 0;
        }
    } else {
        /* Call the Audio Codec Pause/Resume function */
        if (codec_pauseresume(cmd) != 0) {
            return 1;
        } else {
            /* Call the Media layer pause/resume function */
            audio_mal_pauseresume(cmd, addr, size);

            /* Return 0 if all operations are OK */
            return 0;
        }
    }
} 

/*!
    \brief      stops audio playing and power down the audio codec
    \param[in]  option: 
	                This parameter can be any one of the following values:
                        @arg CODEC_PDWN_SW for software power off (by writing registers)
                                           then no need to reconfigure the codec after power on.
                        @arg CODEC_PDWN_HW completely shut down the codec (physically). Then need 
                                           to reconfigure the codec after power on
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t eval_audio_stop(uint32_t option)
{
    /* call audio codec stop function */
    if (codec_stop(option) != 0) {
        return 1;
    } else {
        /* call media layer stop function */
        audio_mal_stop();

        /* update the remaining data number */
        audioremsize = audiototalsize;    

        /* return 0 when all operations are correctly done */
        return 0;
    }
}

/*!
    \brief      controls the current audio volume level
    \param[in]  volume: volume level to be set in percentage from 0% to 100% (0 for Mute and 100 for Max volume level).
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t eval_audio_volumectl(uint8_t volume)
{
    /* Call the codec volume control function with converted volume value */
    return (codec_volumectrl(VOLUME_CONVERT(volume)));
}

/*!
    \brief      enable or disable the MUTE mode by software
    \param[in]  cmd: could be AUDIO_MUTE_ON to mute sound or AUDIO_MUTE_OFF to unmute the codec and restore previous volume level
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t eval_audio_mute(uint32_t cmd)
{
    /* Call the Codec Mute function */
    return (codec_mute(cmd));
}

/*!
    \brief      this function handles main Media layer interrupt
    \param[in]  none
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
void Audio_MAL_IRQHandler(void)
{    
#ifndef AUDIO_MAL_MODE_NORMAL
    uint16_t *pAddr = (uint16_t *)currentpos;
    uint32_t Size = audioremsize;
#endif /* AUDIO_MAL_MODE_NORMAL */
}

/* CS43L22 Audio Codec Control Functions */

/*!
    \brief      initializes the audio codec and all related interfaces (control interface: I2C and audio interface: I2S)
    \param[in]  outputdevice:
                    this parameter can be any one of the following values:
                        @arg OUTPUT_DEVICE_SPEAKER
                        @arg OUTPUT_DEVICE_HEADPHONE
                        @arg OUTPUT_DEVICE_BOTH
                        @arg OUTPUT_DEVICE_AUTO
    \param[in]  volume: initial volume level (from 0 (Mute) to 100 (Max))
    \param[in]  audiofreq: audio frequency used to play the audio stream
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t codec_init(uint16_t outputdevice, uint8_t volume, uint32_t audiofreq)
{
    uint32_t counter = 0;

    /* Configure the Codec related IOs */
    codec_gpio_init();

    /* Reset the Codec Registers */
    codec_reset();

    /* Initialize the Control interface of the Audio Codec */
    codec_ctrlinterface_init();

    /* Keep Codec powered OFF */
    counter += codec_writeregister(0x02, 0x01);

    switch (outputdevice) {
        case OUTPUT_DEVICE_SPEAKER:
            counter += codec_writeregister(0x04, 0xFA); /* SPK always ON & HP always OFF */
            outputdev = 0xFA;
            break;

        case OUTPUT_DEVICE_HEADPHONE:
            counter += codec_writeregister(0x04, 0xAF); /* SPK always OFF & HP always ON */
            outputdev = 0xAF;
            break;

        case OUTPUT_DEVICE_BOTH:
            counter += codec_writeregister(0x04, 0xAA); /* SPK always ON & HP always ON */
            outputdev = 0xAA;
            break;

        case OUTPUT_DEVICE_AUTO:
            counter += codec_writeregister(0x04, 0x05); /* Detect the HP or the SPK automatically */
            outputdev = 0x05;
            break;

        default:
            counter += codec_writeregister(0x04, 0x05); /* Detect the HP or the SPK automatically */
            outputdev = 0x05;
            break;
    }

    /* Clock configuration: Auto detection */  
    counter += codec_writeregister(0x05, 0x81);

    /* Set the Slave Mode and the audio Standard */  
    counter += codec_writeregister(0x06, CODEC_STANDARD);

    /* Set the Master volume */
    codec_volumectrl(volume);

    /* If the Speaker is enabled, set the Mono mode and volume attenuation level */
    if (outputdevice != OUTPUT_DEVICE_HEADPHONE) {
        /* Set the Speaker Mono mode */
        counter += codec_writeregister(0x0F , 0x06);

        /* Set the Speaker attenuation level */
        counter += codec_writeregister(0x24, 0x00);
        counter += codec_writeregister(0x25, 0x00);
    }

    /* Power on the Codec */
    counter += codec_writeregister(0x02, 0x9E);

    /* Additional configuration for the CODEC. These configurations are done to reduce
       the time needed for the Codec to power off. If these configurations are removed,
       then a long delay should be added between powering off the Codec and switching
       off the I2S peripheral MCLK clock (which is the operating clock for Codec).
       If this delay is not inserted, then the codec will not shut down propoerly and
       it results in high noise after shut down. */

    /* Disable the analog soft ramp */
    counter += codec_writeregister(0x0A, 0x00);

    /* Disable the digital soft ramp */
    counter += codec_writeregister(0x0E, 0x04);

    /* Disable the limiter attack level */
    counter += codec_writeregister(0x27, 0x00);

    /* Adjust Bass and Treble levels */
    counter += codec_writeregister(0x1F, 0x0F);

    /* Adjust PCM volume level */
    counter += codec_writeregister(0x1A, 0x0A);
    counter += codec_writeregister(0x1B, 0x0A);

    /* Configure the I2S peripheral */
    codec_audiointerface_init(audiofreq);

    /* Return communication control value */
    return counter;
}

/*!
    \brief      restore the audio codec state to default state and free all used resources
    \param[in]  none
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t codec_deinit(void)
{
    uint32_t counter = 0;

    /* Reset the Codec Registers */
    codec_reset();

    /* Keep Codec powered OFF */
    counter += codec_writeregister(0x02, 0x01);    

    /* Deinitialize all use GPIOs */
    codec_gpio_deinit();

    /* Disable the Codec control interface */
    codec_ctrlinterface_deinit();

    /* Deinitialize the Codec audio interface (I2S) */
    codec_audiointerface_deinit(); 

    /* Return communication control value */
    return counter;  
}

/*!
    \brief      start the audio codec play feature
    \param[in]  none
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t codec_play(void)
{
    /* No actions required on Codec level for play command */

    /* Return communication control value */
    return 0;
}


/*!
    \brief      pauses and resumes playing on the audio codec
    \param[in]  cmd: AUDIO_PAUSE (or 0) to pause, AUDIO_RESUME (or any value different from 0) to resume
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t codec_pauseresume(uint32_t cmd)
{
    uint32_t counter = 0;

    /* Pause the audio file playing */
    if (cmd == AUDIO_PAUSE) {
        /* Mute the output first */
        counter += codec_mute(AUDIO_MUTE_ON);

        /* Put the Codec in Power save mode */
        counter += codec_writeregister(0x02, 0x01);
    } else { /* AUDIO_RESUME */
        /* Unmute the output first */
        counter += codec_mute(AUDIO_MUTE_OFF);

        counter += codec_writeregister(0x04, outputdev);

        /* Exit the Power save mode */
        counter += codec_writeregister(0x02, 0x9E); 
    }

    return counter;
}

/*!
    \brief      stops audio codec playing. it powers down the codec
    \param[in]  codecpdwnmode:
                    this parameter can be any one of the following values:
                        @arg CODEC_PDWN_SW: only mutes the audio codec. when resuming from this mode the 
						                    codec keeps the prvious initialization (no need to re-Initialize the codec registers)
                        @arg CODEC_PDWN_HW: physically power down the codec. when resuming from this mode,
                                            the codec is set to default configuration (user should re-Initialize the codec in order
											to play again the audio stream)
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t codec_stop(uint32_t codecpdwnmode)
{
    uint32_t counter = 0;   

    /* Mute the output first */
    codec_mute(AUDIO_MUTE_ON);

    if (codecpdwnmode == CODEC_PDWN_SW) {
        /* Power down the DAC and the speaker (PMDAC and PMSPK bits)*/
        counter += codec_writeregister(0x02, 0x9F);
    } else {/* CODEC_PDWN_HW */
        /* Power down the DAC components */
        counter += codec_writeregister(0x02, 0x9F);

        /* Wait at least 100ms */
        delay(0xFFF);

        /* Reset The pin */
        //IOE_WriteIOPin(AUDIO_RESET_PIN, BitReset);
    }

    return counter;
}

/*!
    \brief      highers or lowers the codec volume level
    \param[in]  volume: a byte value from 0 to 255 (refer to codec registers description for more details)
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t codec_volumectrl(uint8_t volume)
{
    uint32_t counter = 0;

    if (volume > 0xE6) {
        /* Set the Master volume */
        counter += codec_writeregister(0x20, volume - 0xE7);
        counter += codec_writeregister(0x21, volume - 0xE7);
    } else {
        /* Set the Master volume */
        counter += codec_writeregister(0x20, volume + 0x19);
        counter += codec_writeregister(0x21, volume + 0x19);
    }

    return counter;
}

/*!
    \brief      enables or disables the mute feature on the audio codec
    \param[in]  cmd: AUDIO_MUTE_ON to enable the mute or AUDIO_MUTE_OFF to disable the mute mode
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t codec_mute(uint32_t cmd)
{
    uint32_t counter = 0;

    /* Set the Mute mode */
    if (cmd == AUDIO_MUTE_ON) {
        counter += codec_writeregister(0x04, 0xFF);

        //counter += codec_writeregister(0x0D, 0x63);
        //counter += codec_writeregister(0x0F, 0xF6);
    } else { /* AUDIO_MUTE_OFF Disable the Mute */
        counter += codec_writeregister(0x04, outputdev);

        //counter += codec_writeregister(0x0D, 0x60);
        //counter += codec_writeregister(0x0F, 0x06);
    }

    return counter;
}

/*!
    \brief      reset the audio codec
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void codec_reset(void)
{
    /* Configure the IO Expander (to use the Codec Reset pin mapped on the IOExpander) */
    //IOE_Config();

    /* Power Down the codec */
    //IOE_WriteIOPin(AUDIO_RESET_PIN, BitReset);

    /* wait for a delay to insure registers erasing */
    //delay(CODEC_RESET_DELAY); 

    /* Power on the codec */
    //IOE_WriteIOPin(AUDIO_RESET_PIN, BitSet);
}


/*!
    \brief      switch dynamically (while audio file is played) the output target (speaker or headphone)	
    \param[in]  output:
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
uint32_t codec_switchoutput(uint8_t output)
{
    uint8_t counter = 0;

    switch (output) {
        case OUTPUT_DEVICE_SPEAKER:
            counter += codec_writeregister(0x04, 0xFA); /* SPK always ON & HP always OFF */
            outputdev = 0xFA;
            break;

        case OUTPUT_DEVICE_HEADPHONE:
            counter += codec_writeregister(0x04, 0xAF); /* SPK always OFF & HP always ON */
            outputdev = 0xAF;
            break;

        case OUTPUT_DEVICE_BOTH:
            counter += codec_writeregister(0x04, 0xAA); /* SPK always ON & HP always ON */
            outputdev = 0xAA;
            break;

        case OUTPUT_DEVICE_AUTO:
            counter += codec_writeregister(0x04, 0x05); /* Detect the HP or the SPK automatically */
            outputdev = 0x05;
            break;    

        default:
            counter += codec_writeregister(0x04, 0x05); /* Detect the HP or the SPK automatically */
            outputdev = 0x05;
            break;
    }

    return counter;
}

/*!
    \brief      writes a byte to a given register into the audio codec through the control interface (I2C)
    \param[in]  register_addr: The address (location) of the register to be written
    \param[in]  register_value: the Byte value to be written into destination register
    \param[out] none
    \retval     0 if correct communication, else wrong communication
*/
static uint32_t codec_writeregister(uint32_t register_addr, uint32_t register_value)
{
    uint32_t result = 0;

    /* Return the verifying value: 0 (Passed) or 1 (Failed) */
    return result;
}

/*!
    \brief      none
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void codec_ctrlinterface_init(void)
{

}

/*!
    \brief      restore the Audio Codec control interface to its default state
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void codec_ctrlinterface_deinit(void)
{
    /* Disable the I2C peripheral */ 
    /* This step is not done here because the I2C interface can be used by other modules */
    /* I2C_DeInit(CODEC_I2C); */
}

/*!
    \brief      initializes the Audio Codec audio interface (I2S)
    \param[in]  audiofreq: audio frequency to be configured for the I2S peripheral
    \param[out] none
    \retval     none
*/
static void codec_audiointerface_init(uint32_t audiofreq)
{
    i2s_audiofreq = audiofreq;

    /* enable the CODEC_I2S peripheral clock */
    rcu_periph_clock_enable(CODEC_I2S_CLK);

    /* CODEC_I2S peripheral configuration */
    spi_i2s_deinit(CODEC_I2S);

    /* CODEC_I2S peripheral configuration */
    i2s_psc_config(CODEC_I2S, audiofreq, I2S_FRAMEFORMAT_DT16B_CH16B, 
#ifdef CODEC_MCLK_ENABLED
                   I2S_MCKOUT_ENABLE
#elif defined(CODEC_MCLK_DISABLED)
                   I2S_MCKOUT_DISABLE
#endif
    );

    i2s_init(CODEC_I2S, I2S_MODE_MASTERTX, I2S_STD_LSB, I2S_CKPL_HIGH);

    /* enable the I2S DMA TX request */
    spi_dma_enable(CODEC_I2S, SPI_DMA_TRANSMIT);
    /* The I2S peripheral will be enabled only in the eval_audio_play() function 
       or by user functions if DMA mode not enabled */  
}

/*!
    \brief      restores the Audio Codec audio interface to its default state.
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void codec_audiointerface_deinit(void)
{
    /* disable the CODEC_I2S peripheral (in case it hasn't already been disabled) */
    i2s_disable(CODEC_I2S);

    /* deinitialize the CODEC_I2S peripheral */
    spi_i2s_deinit(CODEC_I2S);

    /* disable the CODEC_I2S peripheral clock */
    rcu_periph_clock_disable(CODEC_I2S_CLK);
}

/*!
    \brief      initializes IOs used by the Audio codec (on the control and audio interfaces).
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void codec_gpio_init(void)
{
    /* enable the GPIO clock */
    rcu_periph_clock_enable(CODEC_I2S_GPIO_CLOCK);
    /* enable I2S0 clock */
    rcu_periph_clock_enable(CODEC_I2S_CLK);
    /* configure GPIO: WS, SCK and SD pins */
    gpio_af_set(CODEC_I2S_GPIO, GPIO_AF_0, CODEC_I2S_WS_PIN | CODEC_I2S_SCK_PIN | CODEC_I2S_SD_PIN);
    gpio_mode_set(CODEC_I2S_GPIO, GPIO_MODE_AF, GPIO_PUPD_NONE, CODEC_I2S_WS_PIN | CODEC_I2S_SCK_PIN | CODEC_I2S_SD_PIN);
    gpio_output_options_set(CODEC_I2S_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, CODEC_I2S_WS_PIN | CODEC_I2S_SCK_PIN | CODEC_I2S_SD_PIN); 

#ifdef CODEC_MCLK_ENABLED
    /* CODEC_I2S pins configuration: MCK pin */
    gpio_af_set(CODEC_I2S_GPIO, GPIO_AF_0, CODEC_I2S_MCK_PIN);
    gpio_mode_set(CODEC_I2S_GPIO, GPIO_MODE_AF, GPIO_PUPD_NONE, CODEC_I2S_MCK_PIN);
    gpio_output_options_set(CODEC_I2S_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, CODEC_I2S_MCK_PIN); 
#endif /* CODEC_MCLK_ENABLED */
}


/*!
    \brief      restores the IO used by the audio codec interface to their default state
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void codec_gpio_deinit(void)
{
    /* deinitialize all the GPIOs used by the driver (EXCEPT the I2C IOs since they are used by the IOExpander as well) */
    gpio_mode_set(CODEC_I2S_GPIO, GPIO_MODE_INPUT, GPIO_OSPEED_50MHZ, CODEC_I2S_WS_PIN | CODEC_I2S_SCK_PIN | CODEC_I2S_SD_PIN);

#ifdef CODEC_MCLK_ENABLED
    /* CODEC_I2S pins deinitialization: MCK pin */
    gpio_mode_set(CODEC_I2S_GPIO, GPIO_MODE_INPUT, GPIO_OSPEED_50MHZ, CODEC_I2S_MCK_PIN);
#endif /* CODEC_MCLK_ENABLED */
}

/*!
    \brief      inserts a delay time (not accurate timing)
    \param[in]  ncount: specifies the delay time length
    \param[out] none
    \retval     none
*/
static void delay( __IO uint32_t ncount)
{
    for (; ncount != 0; ncount--);
}

#ifdef USE_DEFAULT_TIMEOUT_CALLBACK

/*!
    \brief      basic management of the timeout situation
    \param[in]  none
    \param[out] none
    \retval     none
*/
uint32_t Codec_TIMEOUT_UserCallback(void)
{
    /* block communication and all processes */
    while (1)
    {
    }
}
#endif /* USE_DEFAULT_TIMEOUT_CALLBACK */

/* Audio MAL Interface Control Functions */

/*!
    \brief      initializes and prepares the media to perform audio data transfer from media to the I2S peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
void audio_mal_init(void)  
{   
    /* enable the DMA clock */
    rcu_periph_clock_enable(AUDIO_MAL_DMA_CLOCK);

    /* configure the DMA Stream */
    dma_channel_enable(AUDIO_MAL_DMA_CHANNEL);
    dma_deinit(AUDIO_MAL_DMA_CHANNEL);

    /* Set the parameters to be configured */
    dma_initstructure.periph_addr = CODEC_I2S_ADDRESS;
    dma_initstructure.memory_addr = (uint32_t)0;/* This field will be configured in play function */
    dma_initstructure.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_initstructure.number = (uint32_t)0xFFFE;/* This field will be configured in play function */
    dma_initstructure.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_initstructure.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_initstructure.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    dma_initstructure.memory_width = DMA_MEMORY_WIDTH_16BIT;

#ifdef AUDIO_MAL_MODE_NORMAL
    dma_circulation_disable(AUDIO_MAL_DMA_CHANNEL);
#elif defined(AUDIO_MAL_MODE_CIRCULAR)
    dma_circulation_enable(AUDIO_MAL_DMA_CHANNEL);
#else
    #error "AUDIO_MAL_MODE_NORMAL or AUDIO_MAL_MODE_CIRCULAR should be selected !!"
#endif /* AUDIO_MAL_MODE_NORMAL */  

    dma_initstructure.priority = DMA_PRIORITY_HIGH;
    dma_init(AUDIO_MAL_DMA_CHANNEL, &dma_initstructure);

    /* enable the I2S DMA request */
    spi_dma_enable(CODEC_I2S, SPI_DMA_TRANSMIT);
}

/*!
    \brief      restore default state of the used media
    \param[in]  none
    \param[out] none
    \retval     none
*/
void audio_mal_deinit(void)  
{
    /* disable the DMA Channel before the deinit */
    dma_channel_disable(AUDIO_MAL_DMA_CHANNEL);

    /* dinitialize the DMA Channel */
    dma_deinit(AUDIO_MAL_DMA_CHANNEL);

    /* the DMA clock is not disabled, since it can be used by other streams */
}

/*!
    \brief      starts playing audio stream from the audio media
    \param[in]  addr: pointer to the audio stream buffer
    \param[in]  size: number of data in the audio stream buffer
    \param[out] none
    \retval     none
*/
void audio_mal_play(uint32_t addr, uint32_t size)
{
    /* enable the I2S DMA stream */
    dma_channel_disable( AUDIO_MAL_DMA_CHANNEL);

    /* clear the interrupt flag */
    dma_interrupt_flag_clear(AUDIO_MAL_DMA_CHANNEL, DMA_INT_FLAG_FTF);

    /* configure the buffer address and size */
    dma_initstructure.memory_addr = (uint32_t)addr;
    dma_initstructure.number = (uint32_t)(size*2);

    /* configure the DMA stream with the new parameters */
    dma_init(AUDIO_MAL_DMA_CHANNEL, &dma_initstructure);

    /* enable the I2S DMA stream */
    dma_channel_enable(AUDIO_MAL_DMA_CHANNEL);

    /* if the I2S peripheral is still not enabled, enable it */
    if ((SPI_I2SCTL(CODEC_I2S) & I2S_ENABLE_MASK) == 0) {
        i2s_enable(CODEC_I2S);
    }
}


/*!
    \brief      pauses or resumes the audio stream playing from the media
    \param[in]  cmd: AUDIO_PAUSE (or 0) to pause, AUDIO_RESUME (or any value different from 0) to resume
    \param[in]  addr: address from/at which the audio stream should resume/pause	
    \param[in]  size: number of data to be configured for next resume
    \param[out] none
    \retval     none
*/
void audio_mal_pauseresume(uint32_t cmd, uint32_t addr, uint32_t size)
{
    /* pause the audio file playing */
    if (cmd == AUDIO_PAUSE) {
        /* stop the current DMA request by resetting the I2S cell */
        codec_audiointerface_deinit();

        /* Re-configure the I2S interface for the next resume operation */
        codec_audiointerface_init(i2s_audiofreq);

        /* disable the DMA stream */
        dma_channel_disable(AUDIO_MAL_DMA_CHANNEL);

        /* clear the interrupt flag */
        dma_flag_clear(AUDIO_MAL_DMA_CHANNEL, AUDIO_MAL_DMA_FLAG_ALL);

        /* AUDIO_RESUME */
        } else {
        /* configure the buffer address and size */
        dma_initstructure.memory_addr = (uint32_t)addr;
        dma_initstructure.number = (uint32_t)(size*2);

        /* configure the DMA stream with the new parameters */
        dma_init(AUDIO_MAL_DMA_CHANNEL, &dma_initstructure);

        /* enable the I2S DMA stream*/
        dma_channel_enable(AUDIO_MAL_DMA_CHANNEL);

        /* if the i2s peripheral is still not enabled, enable it */
        if ((SPI_I2SCTL(CODEC_I2S) & I2S_ENABLE_MASK) == 0) {
            i2s_enable(CODEC_I2S);
        }
    }
}

/*!
    \brief      stops audio stream playing on the used media
    \param[in]  none
    \param[out] none
    \retval     none
*/
void audio_mal_stop(void)
{
    /* stop the transfer on the i2s side: stop and disable the DMA stream */
    dma_channel_disable(AUDIO_MAL_DMA_CHANNEL);

    /* clear all the DMA flags for the next transfer */
    dma_flag_clear(AUDIO_MAL_DMA_CHANNEL, AUDIO_MAL_DMA_FLAG_ALL);

    /* stop the current DMA request by resetting the i2s cell */
    codec_audiointerface_deinit();

    /* re-configure the i2s interface for the next play operation */
    codec_audiointerface_init(i2s_audiofreq);
}

