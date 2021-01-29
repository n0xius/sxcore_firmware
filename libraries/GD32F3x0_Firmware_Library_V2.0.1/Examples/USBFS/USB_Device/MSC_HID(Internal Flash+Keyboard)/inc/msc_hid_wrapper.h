/*!
    \file  msc_hid_wrapper.h
    \brief the header file of cdc hid wrapper driver

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

#ifndef __CDC_HID_WRAPPER_H
#define __CDC_HID_WRAPPER_H

#include "usbd_msc_core.h"
#include "hid_core.h"

#define HID_INTERFACE                           0x00U
#define MSC_INTERFACE                           0x01U
#define MSC_HID_CONFIG_DESC_SIZE                0x39U

typedef struct
{
    usb_descriptor_configuration_struct        config;

    usb_descriptor_interface_struct            hid_itf;
    usb_hid_descriptor_hid_struct              hid_vendor;
    usb_descriptor_endpoint_struct             hid_epin;
    usb_descriptor_interface_struct            msc_itf;
    usb_descriptor_endpoint_struct             msc_epin;
    usb_descriptor_endpoint_struct             msc_epout;
} usb_descriptor_configuration_set_struct;

extern const usb_descriptor_device_struct msc_hid_dev_desc;
extern const usb_descriptor_configuration_set_struct msc_hid_config_desc;
//extern usb_class_core USBD_msc_hid_cb;
extern uint8_t * usbd_msc_hid_strings[];

extern uint8_t msc_hid_init (void *pudev, uint8_t config_index);
extern uint8_t msc_hid_deinit (void *pudev, uint8_t config_index);
extern uint8_t msc_hid_req_handler (void *pudev, usb_device_req_struct *req);
extern uint8_t msc_hid_data_handler (void *pudev, usb_dir_enum rx_tx, uint8_t ep_id);

#endif  /* __CDC_HID_WRAPPER_H */
