/**
  ******************************************************************************
  * @file    usbd_audio_out_if.c
  * @author  MCU SD
  * @version V1.0.0
  * @date    26-Dec-2014
  * @brief   This file provides the Audio Out (palyback) interface API.
  ******************************************************************************
  */


#include "audio_core.h"
#include "usbd_audio_out_if.h"


static uint8_t  init         (uint32_t audiofreq, uint32_t volume, uint32_t options);
static uint8_t  deinit       (uint32_t options);
static uint8_t  audiocmd     (uint8_t* pbuf, uint32_t size, uint8_t cmd);
static uint8_t  volumectl    (uint8_t vol);
static uint8_t  mutectl      (uint8_t cmd);
static uint8_t  periodictc   (uint8_t cmd);
static uint8_t  getstate     (void);


audio_fops_typedef  audio_out_fops = 
{
    init,
    deinit,
    audiocmd,
    volumectl,
    mutectl,
    periodictc,
    getstate
};

static uint8_t audiostate = AUDIO_STATE_INACTIVE;

/*!
    \brief      initialize and configures all required resources for audio play function
    \param[in]  audiofreq: startup audio frequency
	\param[in]  volume: Startup volume to be set
	\param[in]  options: specific options passed to low layer function
    \param[out] none
    \retval     AUDIO_OK if all operations succeed, AUDIO_FAIL else
*/
static uint8_t  init (uint32_t audiofreq, uint32_t volume, uint32_t options)
{
    static uint32_t Initialized = 0;

    /* Check if the low layer has already been initialized */
    if (Initialized == 0) {
        /* Call low layer function */
        if (eval_audio_init(OUTPUT_DEVICE_AUTO, volume, audiofreq) != 0) {
            audiostate = AUDIO_STATE_ERROR;
            return AUDIO_FAIL;
        }

        /* Set the Initialization flag to prevent reinitializing the interface again */
        Initialized = 1;
    }

    /* Update the Audio state machine */
    audiostate = AUDIO_STATE_ACTIVE;

    return AUDIO_OK;
}

/*!
    \brief      free all resources used by low layer and stops audio-play function
    \param[in]  Options: options passed to low layer function
    \param[out] none
    \retval     AUDIO_OK if all operations succeed, AUDIO_FAIL else
*/
static uint8_t  deinit (uint32_t options)
{
    /* Update the Audio state machine */
    audiostate = AUDIO_STATE_INACTIVE;

    return AUDIO_OK;
}

/*!
    \brief      play, stop, pause or resume current file
	\param[in]  pbuf: address from which file should be played
	\param[in]  size: size of the current buffer/file
    \param[in]  cmd: command to be executed, can be:
                    AUDIO_CMD_PLAY
                    AUDIO_CMD_PAUSE
                    AUDIO_CMD_RESUME
                    AUDIO_CMD_STOP
    \param[out] none
    \retval     AUDIO_OK if all operations succeed, AUDIO_FAIL else
*/
static uint8_t  audiocmd (uint8_t* pbuf, uint32_t size, uint8_t cmd)
{
    /* Check the current state */
    if ((audiostate == AUDIO_STATE_INACTIVE) || (audiostate == AUDIO_STATE_ERROR)) {
        audiostate = AUDIO_STATE_ERROR;

        return AUDIO_FAIL;
    }

    switch (cmd) {
        /* Process the PLAY command ----------------------------*/
        case AUDIO_CMD_PLAY:
            /* If current state is Active or Stopped */
            if ((audiostate == AUDIO_STATE_ACTIVE) || \
                (audiostate == AUDIO_STATE_STOPPED) || \
                (audiostate == AUDIO_STATE_PLAYING)) {
                audio_mal_play((uint32_t)pbuf, (size/2));
                audiostate = AUDIO_STATE_PLAYING;

                return AUDIO_OK;

            /* If current state is Paused */
            } else if (audiostate == AUDIO_STATE_PAUSED) {
                if (eval_audio_pauseresume(AUDIO_RESUME, (uint32_t)pbuf, (size/2)) != 0) {
                    audiostate = AUDIO_STATE_ERROR;

                    return AUDIO_FAIL;
                } else {
                    audiostate = AUDIO_STATE_PLAYING;

                    return AUDIO_OK;
                }

            /* Not allowed command */
            } else {
                return AUDIO_FAIL;
            }

        /* Process the STOP command ----------------------------*/
        case AUDIO_CMD_STOP:
            if (audiostate != AUDIO_STATE_PLAYING) {
                /* Unsupported command */

                return AUDIO_FAIL;
            } else if (eval_audio_stop(CODEC_PDWN_SW) != 0) {
                audiostate = AUDIO_STATE_ERROR;

                return AUDIO_FAIL;
            } else {
                audiostate = AUDIO_STATE_STOPPED;

                return AUDIO_OK;
            }

        /* Process the PAUSE command ---------------------------*/
        case AUDIO_CMD_PAUSE:
            if (audiostate != AUDIO_STATE_PLAYING) {
                /* Unsupported command */
                return AUDIO_FAIL;
            } else if (eval_audio_pauseresume(AUDIO_PAUSE, (uint32_t)pbuf, (size/2)) != 0) {
                audiostate = AUDIO_STATE_ERROR;

                return AUDIO_FAIL;
            } else {
                audiostate = AUDIO_STATE_PAUSED;

                return AUDIO_OK;
            }

        /* Unsupported command ---------------------------------*/
        default:
            return AUDIO_FAIL;
    }
}

/*!
    \brief      set the volume level in %
    \param[in]  vol: volume level to be set in % (from 0% to 100%)
    \param[out] none
    \retval     AUDIO_OK if all operations succeed, AUDIO_FAIL else
*/
static uint8_t  volumectl (uint8_t vol)
{
    /* Call low layer volume setting function */  
    if (eval_audio_volumectl(vol) != 0) {
        audiostate = AUDIO_STATE_ERROR;

        return AUDIO_FAIL;
    }

    return AUDIO_OK;
}

/*!
    \brief      mute or unmute the audio current output
    \param[in]  cmd: can be 0 to unmute, or 1 to mute
    \param[out] none
    \retval     AUDIO_OK if all operations succeed, AUDIO_FAIL else
*/
static uint8_t  mutectl (uint8_t cmd)
{
    /* Call low layer mute setting function */  
    if (eval_audio_mute(cmd) != 0) {
        audiostate = AUDIO_STATE_ERROR;

        return AUDIO_FAIL;
    }

    return AUDIO_OK;
}

/*!
    \brief      none
    \param[in]  cmd: AUDIO_PAUSE (or 0) to pause, AUDIO_RESUME (or any value different from 0) to resume
    \param[out] none
    \retval     AUDIO_OK if all operations succeed, AUDIO_FAIL else
*/
static uint8_t  periodictc (uint8_t cmd)
{
    return AUDIO_OK;
}

/*!
    \brief      return the current state of the audio machine
    \param[in]  none
    \param[out] none
    \retval     current state
*/
static uint8_t  getstate (void)
{
    return audiostate;
}
