/*!
    \file  gd32f3x0_audio_codec.h
    \brief this file contains all the functions prototypes for the audio codec low layer driver

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

#ifndef GD32F3X0_AUDIO_CODEC_H
#define GD32F3X0_AUDIO_CODEC_H


#include "gd32f3x0.h"

/* CONFIGURATION: Audio Codec Driver Configuration parameters */
//#define AUDIO_USE_MACROS

/* Audio Transfer mode (DMA, Interrupt or Polling) */
#define AUDIO_MAL_MODE_NORMAL         /* Uncomment this line to enable the audio 
                                         Transfer using DMA */
/* #define AUDIO_MAL_MODE_CIRCULAR */ /* Uncomment this line to enable the audio 
                                         Transfer using DMA */

/* For the DMA modes select the interrupt that will be used */
/* #define AUDIO_MAL_DMA_IT_TC_EN */  /* Uncomment this line to enable DMA Transfer Complete interrupt */
/* #define AUDIO_MAL_DMA_IT_HT_EN */  /* Uncomment this line to enable DMA Half Transfer Complete interrupt */
/* #define AUDIO_MAL_DMA_IT_TE_EN */  /* Uncomment this line to enable DMA Transfer Error interrupt */

/* #define USE_DMA_PAUSE_FEATURE *//* Uncomment this line to enable the use of DMA Pause Feature
                                 When this define is enabled, the Pause function will
                                 just pause/resume the DMA without need to reinitialize the
                                 DMA structure. In this case, the buffer pointer should remain
                                 available after resume. */

/* Select the interrupt preemption priority and subpriority for the DMA interrupt */
#define EVAL_AUDIO_IRQ_PREPRIO           0   /* Select the preemption priority level(0 is the highest) */
#define EVAL_AUDIO_IRQ_SUBRIO            0   /* Select the sub-priority level (0 is the highest) */

/* Uncomment the following line to use the default Codec_TIMEOUT_UserCallback() 
   function implemented in gd3210c_audio_codec.c file.
   Codec_TIMEOUT_UserCallback() function is called whenever a timeout condition 
   occurs during communication (waiting on an event that doesn't occur, bus 
   errors, busy devices ...). */   
/* #define USE_DEFAULT_TIMEOUT_CALLBACK */


/* OPTIONAL Configuration defines parameters */

/* Uncomment defines below to select standard for audio communication between Codec and I2S peripheral */
/* #define I2S_STANDARD_PHILLIPS */
/* #define I2S_STANDARD_MSB */
#define I2S_STANDARD_LSB

/* Uncomment the defines below to select if the Master clock mode should be enabled or not */
#define CODEC_MCLK_ENABLED
/* #deine CODEC_MCLK_DISABLED */

/* Uncomment this line to enable verifying data sent to codec after each write operation */
/* #define VERIFY_WRITTENDATA */

/* Hardware Configuration defines parameters */

/* I2S peripheral configuration defines */
#define CODEC_I2S                      SPI0
#define CODEC_I2S_CLK                  RCU_SPI0
#define CODEC_I2S_ADDRESS              (SPI0+0x0CU)
#define CODEC_I2S_IRQ                  SPI0_IRQn
#define CODEC_I2S_GPIO_CLOCK           RCU_GPIOA
#define CODEC_I2S_WS_PIN               GPIO_PIN_4
#define CODEC_I2S_SCK_PIN              GPIO_PIN_5
#define CODEC_I2S_SD_PIN               GPIO_PIN_7

#define CODEC_I2S_MCK_PIN              GPIO_PIN_6
#define CODEC_I2S_GPIO                 GPIOA

/* I2S DMA Stream definitions */
#define AUDIO_MAL_DMA_CLOCK            RCU_DMA
#define AUDIO_MAL_DMA_CHANNEL          DMA_CH2
#define AUDIO_MAL_DMA_IRQ              DMA_Channel2_IRQn
#define AUDIO_MAL_DMA_FLAG_ALL         DMA_FLAG_G
#define AUDIO_MAL_DMA_PERIPH_DATA_SIZE DMA_PERIPHERALDATASIZE_HALFWORD
#define AUDIO_MAL_DMA_MEM_DATA_SIZE    DMA_MEMORYDATASIZE_HALFWORD
#define DMA_MAX_SZE                    0xFFFF
#define AUDIO_MAL_DMA                  DMA

#define Audio_MAL_IRQHandler           DMA_Channel2_IRQHandler

/* SPI peripheral configuration defines (control interface of the audio codec) */
//#define CODEC_SPI                      SPI0
//#define CODEC_SPI_CLK                  RCU_SPI0
//#define CODEC_SPI_GPIO_CLOCK           RCU_GPIOA
//#define CODEC_SPI_GPIO_AF              GPIO_AF
//#define CODEC_SPI_GPIO                 GPIOB
//#define CODEC_SPI_SCL_PIN              GPIO_PIN_3
//#define CODEC_SPI_SDA_PIN              GPIO_PIN_5
//#define CODEC_SPI_SEL_PIN              GPIO_PIN_15

/* Maximum Timeout values for flags and events waiting loops. These timeouts are
   not based on accurate values, they just guarantee that the application will 
   not remain stuck if the I2C communication is corrupted.
   You may modify these timeout values depending on CPU frequency and application
   conditions (interrupts routines ...). */   
#define CODEC_FLAG_TIMEOUT             ((uint32_t)0x1000)
#define CODEC_LONG_TIMEOUT             ((uint32_t)(300 * CODEC_FLAG_TIMEOUT))


/* Audio Codec User defines */

/* Codec output DEVICE */
#define OUTPUT_DEVICE_SPEAKER         1
#define OUTPUT_DEVICE_HEADPHONE       2
#define OUTPUT_DEVICE_BOTH            3
#define OUTPUT_DEVICE_AUTO            4

/* Volume Levels values */
#define DEFAULT_VOLMIN                0x00
#define DEFAULT_VOLMAX                0xFF
#define DEFAULT_VOLSTEP               0x04

#define AUDIO_PAUSE                   0
#define AUDIO_RESUME                  1

/* Codec POWER DOWN modes */
#define CODEC_PDWN_HW                 1
#define CODEC_PDWN_SW                 2

/* MUTE commands */
#define AUDIO_MUTE_ON                 1
#define AUDIO_MUTE_OFF                0

#define VOLUME_CONVERT(x)    ((x > 100)? 100:((uint8_t)((x * 255) / 100)))
#define DMA_MAX(x)           (((x) <= DMA_MAX_SZE)? (x):DMA_MAX_SZE)

/* Generic functions */
uint32_t eval_audio_init        (uint16_t outputdevice, uint8_t volume, uint32_t audiofreq);
uint32_t eval_audio_deinit      (void);
uint32_t eval_audio_play        (uint16_t* pbuffer, uint32_t size);
uint32_t eval_audio_pauseresume (uint32_t cmd, uint32_t addr, uint32_t size);
uint32_t eval_audio_stop        (uint32_t codecpowerdown_mode);
uint32_t eval_audio_volumectl   (uint8_t volume);
uint32_t eval_audio_mute        (uint32_t command);

/* Audio Codec functions */

/* High Layer codec functions */
uint32_t codec_init             (uint16_t outputdevice, uint8_t volume, uint32_t audiofreq);
uint32_t codec_deinit           (void);
uint32_t codec_play             (void);
uint32_t codec_pauseresume      (uint32_t cmd);
uint32_t codec_stop             (uint32_t cmd);
uint32_t codec_volumectrl       (uint8_t volume);
uint32_t codec_mute             (uint32_t cmd);
uint32_t codec_switchoutput     (uint8_t output);

/* MAL (Media Access Layer) functions */
void     audio_mal_init         (void);
void     audio_mal_deinit       (void);
void     audio_mal_play         (uint32_t addr, uint32_t size);
void     audio_mal_pauseresume  (uint32_t cmd, uint32_t addr, uint32_t size);
void     audio_mal_stop         (void);

/* User Callbacks: user has to implement these functions in his code if they are needed. */

/* This function is called when the requested data has been completely transferred.
   In Normal mode (when  the define AUDIO_MAL_MODE_NORMAL is enabled) this function
   is called at the end of the whole audio file.
   In circular mode (when  the define AUDIO_MAL_MODE_CIRCULAR is enabled) this 
   function is called at the end of the current buffer transmission. */
void eval_audio_transfercomplete_callback(uint32_t pbuffer, uint32_t size);

/* This function is called when half of the requested buffer has been transferred 
   This callback is useful in Circular mode only (when AUDIO_MAL_MODE_CIRCULAR 
   define is enabled)*/
void eval_audio_halftransfer_callback(uint32_t pbuffer, uint32_t size);

/* This function is called when an Interrupt due to transfer error on or peripheral
   error occurs. */
void eval_audio_error_callback(void* pdata);

/* Codec_TIMEOUT_UserCallback() function is called whenever a timeout condition 
   occurs during communication (waiting on an event that doesn't occur, bus 
   errors, busy devices ...) on the Codec control interface (I2C).
   You can use the default timeout callback implementation by uncommenting the 
   define USE_DEFAULT_TIMEOUT_CALLBACK in audio_codec.h file.
   Typically the user implementation of this callback should reset I2C peripheral
   and re-initialize communication or in worst case reset all the application. */
uint32_t codec_timeout_usercallback(void);
 
#endif /* GD32F3X0_USB_AUDIOCODEC_H */

