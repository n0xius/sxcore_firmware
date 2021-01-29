/*!
    \file  audio_core.c
    \brief USB audio device class core functions

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

#include "usbd_audio_out_if.h"
#include "audio_core.h"
#include "usbd_int.h"
#include "usbd_conf.h"
#include "usb_core.h"

#define USBD_VID                     0x0483
#define USBD_PID                     0x5730

/* Main Buffer for Audio Data Out transfers and its relative pointers */
uint8_t  isocoutbuff [TOTAL_OUT_BUF_SIZE * 2];
uint8_t* isocoutwrptr = isocoutbuff;
uint8_t* isocoutrdptr = isocoutbuff;

/* AUDIO Requests management functions */
static void audio_req_getcurrent   (void *pudev, usb_device_req_struct *req);
static void audio_req_setcurrent   (void *pudev, usb_device_req_struct *req);
static uint8_t  usbd_audio_dataout (void *pudev, uint8_t epid);
static uint8_t  usbd_audio_ep0_rxready (void *pudev);
static uint8_t  usbd_audio_sof (usb_core_handle_struct *pudev);
static void usbd_audio_setinterface(void *pudev, usb_device_req_struct *req);
static void usbd_audio_getinterface(void *pudev, usb_device_req_struct *req);

usbd_int_cb_struct usb_inthandler = 
{
    usbd_audio_sof,
};
usbd_int_cb_struct *usbd_int_fops = &usb_inthandler;

/* Main Buffer for Audio Control Rrequests transfers and its relative variables */
uint8_t  audioctl[64];
uint8_t  audioctlcmd = 0;
uint8_t  audioctlunit = 0;
uint32_t audioctllen = 0;

static uint32_t playflag = 0;

static __IO uint32_t usbd_audio_altset = 0;

/* note:it should use the c99 standard when compiling the below codes */
/* USB standard device descriptor */
const usb_descriptor_device_struct device_descripter =
{
    .Header = 
     {
         .bLength = USB_DEVICE_DESC_SIZE, 
         .bDescriptorType = USB_DESCTYPE_DEVICE
     },
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = USB_MAX_EP0_SIZE,
    .idVendor = USBD_VID,
    .idProduct = USBD_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = USBD_MFC_STR_IDX,
    .iProduct = USBD_PRODUCT_STR_IDX,
    .iSerialNumber = USBD_SERIAL_STR_IDX,
    .bNumberConfigurations = USBD_CFG_MAX_NUM
};

/* USB device configuration descriptor */
const usb_descriptor_configuration_set_struct configuration_descriptor = 
{
    .Config = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_configuration_struct), 
             .bDescriptorType = USB_DESCTYPE_CONFIGURATION 
         },
        .wTotalLength = USB_SPEAKER_CONFIG_DESC_SIZE,
        .bNumInterfaces = 0x02,
        .bConfigurationValue = 0x01,
        .iConfiguration = 0x00,
        .bmAttributes = 0xC0,
        .bMaxPower = 0x32
    },

    .Speaker_Std_Interface = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_interface_struct), 
             .bDescriptorType = USB_DESCTYPE_INTERFACE 
         },
         .bInterfaceNumber = 0x00,
         .bAlternateSetting = 0x00,
         .bNumEndpoints = 0x00,
         .bInterfaceClass = 0x01,
         .bInterfaceSubClass = 0x01,
         .bInterfaceProtocol = 0x00,
         .iInterface = 0x00
    },
    
    .Speaker_AC_Interface = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_AC_interface_struct), 
             .bDescriptorType = 0x24 
         },
         .bDescriptorSubtype = 0x01,
         .bcdADC = 0x0100,
         .wTotalLength = 0x0027,
         .bInCollection = 0x01,
         .baInterfaceNr = 0x01
    },
    
    .Speaker_IN_Terminal = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_input_terminal_struct), 
             .bDescriptorType = 0x24 
         },
         .bDescriptorSubtype = 0x02,
         .bTerminalID = 0x01,
         .wTerminalType = 0x0101,
         .bAssocTerminal = 0x00,
         .bNrChannels = 0x01,
         .wChannelConfig = 0x0000,
         .iChannelNames = 0x00,
         .iTerminal = 0x00
    },
    
    .Speaker_Feature_Unit = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_mono_feature_unit_struct), 
             .bDescriptorType = 0x24 
         },
         .bDescriptorSubtype = AUDIO_CONTROL_FEATURE_UNIT,
         .bUnitID = AUDIO_OUT_STREAMING_CTRL,
         .bSourceID = 0x01,
         .bControlSize = 0x01,
         .bmaControls0 = AUDIO_CONTROL_MUTE,
         .bmaControls1 = 0x00,
         .iFeature = 0x00
    },
    
    .Speaker_OUT_Terminal = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_output_terminal_struct), 
             .bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE 
         },
         .bDescriptorSubtype = AUDIO_CONTROL_OUTPUT_TERMINAL,
         .bTerminalID = 0x03,
         .wTerminalType = 0x0301,
         .bAssocTerminal = 0x00,
         .bSourceID = 0x02,
         .iTerminal = 0x00
    },

    .Speaker_Std_AS_Interface_ZeroBand = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_interface_struct), 
             .bDescriptorType = USB_DESCTYPE_INTERFACE 
         },
         .bInterfaceNumber = 0x01,
         .bAlternateSetting = 0x00,
         .bNumEndpoints = 0x00,
         .bInterfaceClass = USB_DEVICE_CLASS_AUDIO,
         .bInterfaceSubClass = AUDIO_SUBCLASS_AUDIOSTREAMING,
         .bInterfaceProtocol = AUDIO_PROTOCOL_UNDEFINED,
         .iInterface = 0x00
    },
    
    .Speaker_Std_AS_Interface_Opera = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_interface_struct), 
             .bDescriptorType = USB_DESCTYPE_INTERFACE 
         },
         .bInterfaceNumber = 0x01,
         .bAlternateSetting = 0x01,
         .bNumEndpoints = 0x01,
         .bInterfaceClass = USB_DEVICE_CLASS_AUDIO,
         .bInterfaceSubClass = AUDIO_SUBCLASS_AUDIOSTREAMING,
         .bInterfaceProtocol = AUDIO_PROTOCOL_UNDEFINED,
         .iInterface = 0x00
    },
    
    .Speaker_AS_Interface = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_AS_interface_struct), 
             .bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE 
         },
         .bDescriptorSubtype = AUDIO_STREAMING_GENERAL,
         .bTerminalLink = 0x01,
         .bDelay = 0x01,
         .wFormatTag = 0x0001,
    },
    
    .Speaker_Format_TypeI = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_format_type_struct), 
             .bDescriptorType = AUDIO_INTERFACE_DESCRIPTOR_TYPE 
         },
         .bDescriptorSubtype = AUDIO_STREAMING_FORMAT_TYPE,
         .bFormatType = AUDIO_FORMAT_TYPE_III,
         .bNrChannels = 0x02,
         .bSubFrameSize = 0x02,
         .bBitResolution = 0x10,
         .bSamFreqType = 0x01,
         .bSamFreq[0]= (uint8_t)USBD_AUDIO_FREQ,
         .bSamFreq[1]= USBD_AUDIO_FREQ >> 8,
         .bSamFreq[2]= USBD_AUDIO_FREQ >> 16
    },
    
    .Speaker_Std_Endpoint = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_std_endpoint_struct), 
             .bDescriptorType = USB_DESCTYPE_ENDPOINT 
         },
         .bEndpointAddress = AUDIO_OUT_EP,
         .bmAttributes = USB_ENDPOINT_TYPE_ISOCHRONOUS,
         .wMaxPacketSize = PACKET_SIZE(USBD_AUDIO_FREQ),
         .bInterval = 0x01,
         .bRefresh = 0x00,
         .bSynchAddress = 0x00
    },
    
    .Speaker_AS_Endpoint = 
    {
        .Header = 
         {
             .bLength = sizeof(usb_descriptor_AS_endpoint_struct), 
             .bDescriptorType = AUDIO_ENDPOINT_DESCRIPTOR_TYPE 
         },
         .bDescriptorSubtype = AUDIO_ENDPOINT_GENERAL,
         .bmAttributes = 0x00,
         .bLockDelayUnits = 0x00,
         .wLockDelay = 0x0000,
    }
};

/* USB language ID Descriptor */
const usb_descriptor_language_id_struct usbd_language_id_desc = 
{
    .Header = 
     {
         .bLength = sizeof(usb_descriptor_language_id_struct), 
         .bDescriptorType = USB_DESCTYPE_STRING
     },
    .wLANGID = ENG_LANGID
};

/* USB serial string */
uint8_t usbd_serial_string[USB_SERIAL_STRING_SIZE] =
{
    USB_SERIAL_STRING_SIZE,
    USB_DESCTYPE_STRING,
};

__ALIGN_BEGIN uint8_t* usbd_strings[] __ALIGN_END = 
{
    [USBD_LANGID_STR_IDX] = (uint8_t *)&usbd_language_id_desc,
    [USBD_MFC_STR_IDX] = USBD_STRING_DESC("GD32_Microelectronics"),
    [USBD_PRODUCT_STR_IDX] = USBD_STRING_DESC("GD32 Audio in FS Mode"),
    [USBD_SERIAL_STR_IDX] = USBD_STRING_DESC("GD32F3X0-2.0.0-6f7e8dma"),
};

/*!
    \brief      initialize the AUDIO device
    \param[in]  pudev: pointer to usb device instance
    \param[in]  config_index: configuration index
    \param[out] none
    \retval     usb device operation status
*/
uint8_t audio_init (void *pudev, uint8_t config_index)
{
    /* initialize Rx endpoint */
    usbd_ep_init(pudev, (const usb_descriptor_endpoint_struct *)&(configuration_descriptor.Speaker_Std_Endpoint));

    /* Initialize the Audio output Hardware layer */
    if (audio_out_fops.init(USBD_AUDIO_FREQ, DEFAULT_VOLUME, 0) != USBD_OK) {
        return USBD_FAIL;
    }

    /* Prepare Out endpoint to receive audio data */
    usbd_ep_rx (pudev, AUDIO_OUT_EP, (uint8_t*)isocoutbuff, AUDIO_OUT_PACKET);

    return USBD_OK;
}

/*!
    \brief      de-initialize the AUDIO device
    \param[in]  pudev: pointer to usb device instance
    \param[in]  config_index: configuration index
    \param[out] none
    \retval     usb device operation status
*/
uint8_t audio_deinit (void *pudev, uint8_t config_index)
{
    /* deinitialize AUDIO endpoints */
    usbd_ep_deinit(pudev, AUDIO_OUT_EP);

    /* DeInitialize the Audio output Hardware layer */
    if (audio_out_fops.deinit(0) != USBD_OK) {
        return USBD_FAIL;
    }

    return USBD_OK;
}

/*!
    \brief      handle the AUDIO class-specific requests
    \param[in]  pudev: pointer to usb device instance
    \param[in]  req: device class-specific request
    \param[out] none
    \retval     usb device operation status
*/
uint8_t audio_req_handler (void *pudev, usb_device_req_struct *req)
{
    switch (req->bmRequestType & USB_REQ_MASK) {
    case USB_CLASS_REQ:
        switch (req->bRequest) {
            case AUDIO_REQ_GET_CUR:
                audio_req_getcurrent(pudev, req);
                break;

            case AUDIO_REQ_SET_CUR:
                audio_req_setcurrent(pudev, req);
                break;

            default:
                usbd_enum_error (pudev, req);
                return USBD_FAIL; 
        }
        
    case USB_STANDARD_REQ:
        /* standard device request */
        switch(req->bRequest) {
            case USBREQ_GET_INTERFACE:
                usbd_audio_getinterface(pudev, req);
                break;

            case USBREQ_SET_INTERFACE:
                usbd_audio_setinterface(pudev, req);
                usbd_ctlstatus_tx(pudev);
                break;

            default:
                break;
        }
        break;

    default:
        break;
    }

    return USBD_OK;
}

/*!
    \brief      handle data stage
    \param[in]  pudev: pointer to usb device instance
    \param[in]  rx_tx: the flag of Rx or Tx
    \param[in]  ep_id: the endpoint ID
    \param[out] none
    \retval     usb device operation status
*/
uint8_t audio_data_handler (void *pudev, usb_dir_enum rx_tx, uint8_t ep_id)
{
    if ((USB_TX == rx_tx) && ((AUDIO_IN_EP & 0x7F) == ep_id)) {
      
        return USBD_OK;
    } else if ((USB_RX == rx_tx) && ((EP0_OUT & 0x7F) == ep_id)) {
    
        usbd_audio_ep0_rxready(pudev);
        return USBD_OK;
    } else if ((USB_RX == rx_tx) && ((AUDIO_OUT_EP & 0x7F) == ep_id)) {
      
        usbd_audio_dataout(pudev, ep_id);
        return USBD_OK;
    }
    return USBD_FAIL;
}

/**
  * @brief  Handles the Audio Out data stage.
  * @param  pudev: pointer to usb device instance
  * @param  epid: endpoint identifer
  * @retval usb device operation status
  */
static uint8_t  usbd_audio_dataout (void *pudev, uint8_t epid)
{
    if (epid == AUDIO_OUT_EP) {
        /* Increment the Buffer pointer or roll it back when all buffers are full */
        if (isocoutwrptr >= (isocoutbuff + (AUDIO_OUT_PACKET * OUT_PACKET_NUM))) {
            /* All buffers are full: roll back */
            isocoutwrptr = isocoutbuff;
        } else {
            /* Increment the buffer pointer */
            isocoutwrptr += AUDIO_OUT_PACKET;
        }

        /* Toggle the frame index */  
        ((usb_core_handle_struct*)pudev)->dev.out_ep[epid].endp_frame = 
        (((usb_core_handle_struct*)pudev)->dev.out_ep[epid].endp_frame)? 0:1;

        /* Prepare Out endpoint to receive next audio packet */
        usbd_ep_rx (pudev, AUDIO_OUT_EP, (uint8_t*)(isocoutwrptr), AUDIO_OUT_PACKET);

        /* Trigger the start of streaming only when half buffer is full */
        if ((playflag == 0) && (isocoutwrptr >= (isocoutbuff + ((AUDIO_OUT_PACKET * OUT_PACKET_NUM) / 2)))) {
            /* Enable start of Streaming */
            playflag = 1;
        }
    }

    return USBD_OK;
}

/**
  * @brief  Handles audio control requests data.
  * @param  pudev: pointer to usb device instance
  * @retval usb device operation status
  */
static uint8_t  usbd_audio_ep0_rxready (void *pudev)
{
    /* Check if an AudioControl request has been issued */
    if (audioctlcmd == AUDIO_REQ_SET_CUR) {/* In this driver, to simplify code, only SET_CUR request is managed */

        /* Check for which addressed unit the AudioControl request has been issued */
        if (audioctlunit == AUDIO_OUT_STREAMING_CTRL) {/* In this driver, to simplify code, only one unit is manage */

            /* Call the audio interface mute function */
            audio_out_fops.mutectl(audioctl[0]);

            /* Reset the audioctlcmd variable to prevent re-entering this function */
            audioctlcmd = 0;
            audioctllen = 0;
        }
    }

    return USBD_OK;
}

/**
  * @brief  Handles the SOF event (data buffer update and synchronization).
  * @param  pudev: pointer to usb device instance
  * @retval usb device operation status
  */
static uint8_t  usbd_audio_sof (usb_core_handle_struct *pudev)
{
    /* Check if there are available data in stream buffer.
       In this function, a single variable (playflag) is used to avoid software delays.
       The play operation must be executed as soon as possible after the SOF detection. */
    if (playflag) {
        /* Start playing received packet */
        audio_out_fops.audiocmd((uint8_t*)(isocoutrdptr),  /* Samples buffer pointer */
                                AUDIO_OUT_PACKET,          /* Number of samples in Bytes */
                                AUDIO_CMD_PLAY);           /* Command to be processed */

        /* Increment the Buffer pointer or roll it back when all buffers all full */  
        if (isocoutrdptr >= (isocoutbuff + (AUDIO_OUT_PACKET * OUT_PACKET_NUM))) {
            /* Roll back to the start of buffer */
            isocoutrdptr = isocoutbuff;
        } else {
            /* Increment to the next sub-buffer */
            isocoutrdptr += AUDIO_OUT_PACKET;
        }

        /* If all available buffers have been consumed, stop playing */
        if (isocoutrdptr == isocoutwrptr) {
            /* Pause the audio stream */
            audio_out_fops.audiocmd((uint8_t*)(isocoutbuff),   /* Samples buffer pointer */
                                    AUDIO_OUT_PACKET,          /* Number of samples in Bytes */
                                    AUDIO_CMD_PAUSE);          /* Command to be processed */

            /* Stop entering play loop */
            playflag = 0;

            /* Reset buffer pointers */
            isocoutrdptr = isocoutbuff;
            isocoutwrptr = isocoutbuff;
        }
    }

    return USBD_OK;
}

/**
  * @brief  Handle standard device request--Get Interface
  * @param  pudev: pointer to usb device instance
  * @param  req: standard device request
  * @retval usb device operation status
  */
static void usbd_audio_getinterface (void *pudev, usb_device_req_struct *req)
{
    usbd_ctltx(pudev, (uint8_t *)&usbd_audio_altset, 1);
}

/**
  * @brief  Handle standard device request--Set Interface
  * @param  pudev: pointer to usb device instance
  * @param  req: standard device request
  * @retval usb device operation status
  */
static void usbd_audio_setinterface (void *pudev, usb_device_req_struct *req)
{
    if ((uint8_t)(req->wValue) < AUDIO_TOTAL_IF_NUM) {
        usbd_audio_altset = (uint8_t)(req->wValue);
    } else {
        usbd_ep_stall(pudev,0x80);
        usbd_ep_stall(pudev,0x00);
        usb_ep0_startout(pudev);
    }
}

/**
  * @brief  Handles the GET_CUR Audio control request.
  * @param  pudev: pointer to usb device instance
  * @param  req: setup class request
  * @retval usb device operation status
  */
static void audio_req_getcurrent(void *pudev, usb_device_req_struct *req)
{
    /* Send the current mute state */
    usbd_ctltx(pudev, audioctl, req->wLength);
}

/**
  * @brief  Handles the SET_CUR Audio control request.
  * @param  pudev: pointer to usb device instance
  * @param  req: setup class request
  * @retval usb device operation status
  */
static void audio_req_setcurrent(void *pudev, usb_device_req_struct *req)
{
    if (req->wLength) {
        /* Prepare the reception of the buffer over EP0 */
        usbd_ctlrx (pudev, audioctl, req->wLength);
        
        /* Set the global variables indicating current request and its length 
        to the function usbd_audio_EP0_RxReady() which will process the request */
        audioctlcmd = AUDIO_REQ_SET_CUR;     /* Set the request value */
        audioctllen = req->wLength;          /* Set the request data length */
        audioctlunit = HIGHBYTE(req->wIndex);  /* Set the request target unit */
    }
}


