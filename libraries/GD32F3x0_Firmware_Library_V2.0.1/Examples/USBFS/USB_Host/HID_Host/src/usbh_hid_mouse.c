/*!
    \file  usbh_hid_mouse.c 
    \brief this file is the application layer for usb host hid mouse handling

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

#include "usbh_hid_mouse.h"
#include "gd32f3x0_lcd_eval.h"

static void mouse_init    (void);
static void mouse_decode  (uint8_t *data);

int16_t  x_loc = 0, y_loc = 0;
int16_t  prev_x = 0, prev_y = 0;

hid_mouse_data_struct hid_mouse_data;

hid_cb_struct hid_mouse_cb = 
{
    mouse_init,
    mouse_decode,
};

/*!
    \brief      init mouse state.
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void  mouse_init (void)
{
    /* call user init */
    usr_mouse_init();
}

/*!
    \brief      decode mouse data
    \param[in]  data: pointer to mouse hid data buffer
    \param[out] none
    \retval     none
*/
static void  mouse_decode(uint8_t *data)
{
    hid_mouse_data.button = data[0];

    hid_mouse_data.x = data[1];
    hid_mouse_data.y = data[2];

    usr_mouse_process_data(&hid_mouse_data);
}

/*!
    \brief      handle mouse scroll to upadte the mouse position on display window
    \param[in]  x: USB HID mouse X co-ordinate
    \param[in]  y: USB HID mouse Y co-ordinate
    \param[out] none
    \retval     none
*/
void hid_mouse_position_update (int8_t x, int8_t y)
{
    if ((x != 0) || (y != 0)) {
        x_loc += x / 2;
        y_loc += y / 2;

        if (y_loc > MOUSE_WINDOW_HEIGHT - 12) {
            y_loc = MOUSE_WINDOW_HEIGHT - 12;
        }

        if (x_loc > MOUSE_WINDOW_WIDTH - 16) {
            x_loc = MOUSE_WINDOW_WIDTH - 16;
        }

        if (y_loc < 2) {
            y_loc = 2;
        }

        if (x_loc < 2) {
            x_loc = 2;
        }

        if ((prev_x != 0) && (prev_y != 0)) {
            lcd_draw_font_gbk16 (MOUSE_WINDOW_X + prev_y, 
                                 MOUSE_WINDOW_Y + prev_x, 
                                 BLACK, BLACK, (char *)"x");
        } else if((prev_x == 0) && ( prev_y == 0)) {
            lcd_draw_font_gbk16 (MOUSE_WINDOW_X, 
                                 MOUSE_WINDOW_Y, 
                                 BLACK, BLACK, (char *)"x");
        }

        lcd_draw_font_gbk16 (MOUSE_WINDOW_X + y_loc, 
                             MOUSE_WINDOW_Y + x_loc, 
                             GREEN, BLACK, (char *)"x");

        prev_x = x_loc;
        prev_y = y_loc;
    }
}

/*!
    \brief      handle mouse button press
    \param[in]  button_idx : mouse button pressed
    \param[out] none
    \retval     none
*/
void hid_mouse_button_pressed(uint8_t button_idx)
{
    /* change the color of button pressed to indicate button press*/
    switch (button_idx) {
    /* left button pressed */
    case HID_MOUSE_LEFTBUTTON:
        lcd_rect_color_draw(20, 220, 60, 260, GREEN);
        break;
    /* right button pressed */  
    case HID_MOUSE_RIGHTBUTTON:
        lcd_rect_color_draw(180, 220, 220, 260, GREEN);
        break;
    /* middle button pressed */
    case HID_MOUSE_MIDDLEBUTTON:
        lcd_rect_color_draw(100, 220, 140, 260, GREEN);
        break;
    default:
        break;
    }
}

/*!
    \brief      handle mouse button release
    \param[in]  button_idx : mouse button pressed
    \param[out] none
    \retval     none
*/
void hid_mouse_button_released(uint8_t button_idx)
{
    /* change the color of button released to default button color*/
    switch (button_idx) {
    /* left button released */
    case HID_MOUSE_LEFTBUTTON:
        lcd_rect_color_draw(20, 220, 60, 260, WHITE);
        break;
    /* right button released */
    case HID_MOUSE_RIGHTBUTTON:
        lcd_rect_color_draw(180, 220, 220, 260, WHITE);
        break;
    /* middle button released */
    case HID_MOUSE_MIDDLEBUTTON:
        lcd_rect_color_draw(100, 220, 140, 260, WHITE);
        break;
    default:
        break;
    }
}
