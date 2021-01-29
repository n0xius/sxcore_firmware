/*!
    \file  usbh_hid_mouse.h 
    \brief this file contains all the prototypes for the usbh_hid_mouse.c

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

#ifndef USBH_HID_MOUSE_H
#define USBH_HID_MOUSE_H

#include "usbh_hid_core.h"

/* Left Button: Report data: 0x00*/
#define HID_MOUSE_LEFTBUTTON                    0x00

/* Right Button: Report data: 0x01*/
#define HID_MOUSE_RIGHTBUTTON                   0x01

/* Middle Button: Report data: 0x02*/
#define HID_MOUSE_MIDDLEBUTTON                  0x02

#define MOUSE_WINDOW_X                          20
#define MOUSE_WINDOW_Y                          60
#define MOUSE_WINDOW_HEIGHT                     200
#define MOUSE_WINDOW_WIDTH                      140

typedef struct
{
    uint8_t              x; 
    uint8_t              y;
    uint8_t              z;               /* not supported */ 
    uint8_t              button; 
}hid_mouse_data_struct;

extern hid_cb_struct hid_mouse_cb;
extern hid_mouse_data_struct hid_mouse_data;

/* function declarations */
/* init mouse window */
void usr_mouse_init (void);
/* process mouse data */
void usr_mouse_process_data (hid_mouse_data_struct *data);
/* handle mouse scroll to upadte the mouse position on display window */
void hid_mouse_position_update (int8_t x, int8_t y);
/* handle mouse button press */
void hid_mouse_button_pressed(uint8_t button_idx);
/* handle mouse button release */
void hid_mouse_button_released(uint8_t button_idx);

#endif /* USBH_HID_MOUSE_H */
