/*!
    \file  usbd_audio_out_if.h
    \brief the header file of the usbd_audio_out_if.c file

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

#ifndef __USBD_AUDIO_OUT_IF_H
#define __USBD_AUDIO_OUT_IF_H

#include "gd32f3x0_audio_codec.h"
#include "audio_core.h"

/* audio commands enumeration */
typedef enum
{
    AUDIO_CMD_PLAY = 1,
    AUDIO_CMD_PAUSE,
    AUDIO_CMD_STOP,
}audio_cmd_typedef;

/* Mute commands */
#define AUDIO_MUTE                      0x01
#define AUDIO_UNMUTE                    0x00

/* Functions return value */
#define AUDIO_OK                        0x00
#define AUDIO_FAIL                      0xFF

/* Audio Machine States */
#define AUDIO_STATE_INACTIVE            0x00
#define AUDIO_STATE_ACTIVE              0x01
#define AUDIO_STATE_PLAYING             0x02
#define AUDIO_STATE_PAUSED              0x03
#define AUDIO_STATE_STOPPED             0x04
#define AUDIO_STATE_ERROR               0x05

extern audio_fops_typedef  audio_out_fops;

#endif  /* __USBD_AUDIO_OUT_IF_H */
  

