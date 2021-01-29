/*!
    \file  msc_hid_wrapper.c
    \brief this file calls to the separate CDC and HID class layer handlers

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

#include "msc_hid_wrapper.h"
#include "usbd_int.h"

#define USBD_VID                          0x28E9U
#define USBD_PID                          0x3258U

usbd_int_cb_struct *usbd_int_fops = NULL;

/* note:it should use the C99 standard when compiling the below codes */
/* USB standard device descriptor */
const usb_descriptor_device_struct msc_hid_dev_desc =
{
    .Header = 
     {
         .bLength          = USB_DEVICE_DESC_SIZE, 
         .bDescriptorType  = USB_DESCTYPE_DEVICE,
     },
    .bcdUSB                = 0x0200U,
    .bDeviceClass          = 0x00U,
    .bDeviceSubClass       = 0x00U,
    .bDeviceProtocol       = 0x00U,
    .bMaxPacketSize0       = USB_MAX_EP0_SIZE,
    .idVendor              = USBD_VID,
    .idProduct             = USBD_PID,
    .bcdDevice             = 0x0100U,
    .iManufacturer         = USBD_MFC_STR_IDX,
    .iProduct              = USBD_PRODUCT_STR_IDX,
    .iSerialNumber         = USBD_SERIAL_STR_IDX,
    .bNumberConfigurations = USBD_CFG_MAX_NUM,
};

/* USB device configuration descriptor */
const usb_descriptor_configuration_set_struct msc_hid_config_desc = 
{
    .config = 
    {
        .Header = 
         {
             .bLength         = sizeof(usb_descriptor_configuration_struct), 
             .bDescriptorType = USB_DESCTYPE_CONFIGURATION,
         },
        .wTotalLength         = MSC_HID_CONFIG_DESC_SIZE,
        .bNumInterfaces       = 0x02U,
        .bConfigurationValue  = 0x01U,
        .iConfiguration       = 0x00U,
        .bmAttributes         = 0xE0U,
        .bMaxPower            = 0x32U
    },

    .hid_itf = 
    {
        .Header = 
         {
             .bLength         = sizeof(usb_descriptor_interface_struct), 
             .bDescriptorType = USB_DESCTYPE_INTERFACE
         },
        .bInterfaceNumber     = HID_INTERFACE,
        .bAlternateSetting    = 0x00U,
        .bNumEndpoints        = 0x01U,
        .bInterfaceClass      = 0x03U,
        .bInterfaceSubClass   = 0x01U,
        .bInterfaceProtocol   = 0x01U,
        .iInterface           = 0x00U
    },

    .hid_vendor = 
    {
        .Header = 
         {
             .bLength         = sizeof(usb_hid_descriptor_hid_struct),
             .bDescriptorType = HID_DESC_TYPE 
         },
        .bcdHID               = 0x0111U,
        .bCountryCode         = 0x00U,
        .bNumDescriptors      = 0x01U,
        .bDescriptorType      = HID_REPORT_DESCTYPE,
        .wDescriptorLength    = USB_HID_REPORT_DESC_SIZE,
    },

    .hid_epin = 
    {
        .Header = 
         {
             .bLength         = sizeof(usb_descriptor_endpoint_struct), 
             .bDescriptorType = USB_DESCTYPE_ENDPOINT 
         },
        .bEndpointAddress     = HID_IN_EP,
        .bmAttributes         = 0x03U,
        .wMaxPacketSize       = HID_IN_PACKET,
        .bInterval            = 0x40U
    },

//    .hid_epout = 
//    {
//        .Header = 
//         {
//             .bLength         = sizeof(usb_descriptor_endpoint_struct), 
//             .bDescriptorType = USB_DESCTYPE_ENDPOINT
//         },
//        .bEndpointAddress     = HID_OUT_EP,
//        .bmAttributes         = 0x03U,
//        .wMaxPacketSize       = HID_OUT_PACKET,
//        .bInterval            = 0x20U
//    },

    .msc_itf = 
    {
        .Header = {
            .bLength         = sizeof(usb_descriptor_interface_struct), 
            .bDescriptorType = USB_DESCTYPE_INTERFACE
        },
        .bInterfaceNumber    = MSC_INTERFACE,
        .bAlternateSetting   = 0x00U,
        .bNumEndpoints       = 0x02U,
        .bInterfaceClass     = 0x08U,
        .bInterfaceSubClass  = 0x06U,
        .bInterfaceProtocol  = 0x50U,
        .iInterface          = 0x01U
    },

    .msc_epin = 
    {
        .Header = {
            .bLength         = sizeof(usb_descriptor_endpoint_struct), 
            .bDescriptorType = USB_DESCTYPE_ENDPOINT
        },
        .bEndpointAddress    = MSC_IN_EP,
        .bmAttributes        = 0x02U,
        .wMaxPacketSize      = MSC_EPIN_SIZE,
        .bInterval           = 0x00U
    },

    .msc_epout = 
    {
        .Header = {
            .bLength         = sizeof(usb_descriptor_endpoint_struct), 
            .bDescriptorType = USB_DESCTYPE_ENDPOINT
        },
        .bEndpointAddress    = MSC_OUT_EP,
        .bmAttributes        = 0x02U,
        .wMaxPacketSize      = MSC_EPOUT_SIZE,
        .bInterval           = 0x00U
    }
};

/* USB language ID Descriptor */
static const usb_descriptor_language_id_struct usbd_language_id_desc = 
{
    .Header = 
     {
         .bLength         = sizeof(usb_descriptor_language_id_struct), 
         .bDescriptorType = USB_DESCTYPE_STRING,
     },
    .wLANGID              = ENG_LANGID
};

uint8_t * usbd_msc_hid_strings[] = 
{
    [USBD_LANGID_STR_IDX]  = (uint8_t *)&usbd_language_id_desc,
    [USBD_MFC_STR_IDX]     = USBD_STRING_DESC("GigaDevice"),
    [USBD_PRODUCT_STR_IDX] = USBD_STRING_DESC("GigaDevice USB MSC HID device !"),
    [USBD_SERIAL_STR_IDX]  = USBD_STRING_DESC("GGD32-V1.0.0-4b5c6d7e")
};

/*!
    \brief      initialize the HID/CDC device
    \param[in]  pudev: pointer to USB device instance
    \param[in]  config_index: configuration index
    \param[out] none
    \retval     USB device operation status
*/
uint8_t msc_hid_init (void *pudev, uint8_t config_index)
{
    /* HID initialization */
    usbd_hid_init(pudev, config_index);

    /* CDC initialization */
    msc_init(pudev, config_index);

    return USBD_OK;
}

/*!
    \brief      de-initialize the HID/CDC device
    \param[in]  pudev: pointer to USB device instance
    \param[in]  config_index: configuration index
    \param[out] none
    \retval     USB device operation status
*/
uint8_t msc_hid_deinit (void *pudev, uint8_t config_index)
{
    /* HID De-initialization */
    usbd_hid_deinit(pudev, config_index);

    /* MSC De-initialization */
    msc_deinit(pudev, config_index);

    return USBD_OK;
}

/*!
    \brief      handle the custom HID/CDC class-specific request
    \param[in]  pudev: pointer to USB device instance
    \param[in]  req: device class request
    \param[out] none
    \retval     USB device operation status
*/
uint8_t msc_hid_req_handler (void *pudev, usb_device_req_struct *req)
{
    if (req->wIndex == HID_INTERFACE)
    {
        return usbd_hid_classreq_handle(pudev, req);
    } else {
        return msc_req_handler(pudev, req);
    }
}

/*!
    \brief      handle data stage
    \param[in]  pudev: pointer to USB device instance
    \param[in]  rx_tx: data transfer direction:
      \arg        USBD_TX
      \arg        USBD_RX
    \param[in]  ep_id: endpoint identifier
    \param[out] none
    \retval     USB device operation status
*/
uint8_t msc_hid_data_handler (void *pudev, usb_dir_enum rx_tx, uint8_t ep_id)
{
    if ((HID_IN_EP & 0x7F) == ep_id || ep_id == (HID_OUT_EP & 0x7F)) {
        return usbd_hid_data_handler(pudev, rx_tx, ep_id);
    } else {
        return msc_data_handler(pudev, rx_tx, ep_id);
    }
}
