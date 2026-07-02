/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef COMMON_H
#define COMMON_H

#include "audio_common.h"
#include "audio_if_lib_common.h"
#include "hdf_io_service_if.h"
#include "asoundlib.h"
#include "osal_mem.h"
#include "hdf_sbuf.h"
#include "audio_uhdf_log.h"
#include "securec.h"
#include "local.h"
#include "alsa_soundcard.h"

#ifndef ALSA_SND_COMMON_H
#define ALSA_SND_COMMON_H

/* Different scenarios correspond to different devices */
#define     SND_DEFAULT_PCM_DEV             (-1)
#define     SND_CALL_PCM_DEV                4
#define     SND_CALL_CAPTURE_DEV            "hw:0,5"

#define BIT_COUNT_OF_BYTE                   8
#define PCM_WAIT_TIMEOUT_MS                 100

#define AUDIO_PERIOD_TIME_RATIO             4
#define AUDIO_VOLUME_TO_HDI_FLOAT_DIVISOR   15
#define AUDIO_VOLUME_TO_HDI_DIVISOR         7
#define VOLUME_LEN                          3
#define VOICE_VOLUME_PATH                   "VBC_VOLUME"
#define VOICE_VOLUME_ID                     97
#define CAPTURE_MUTE_PATH                   "VBC_UL_MUTE"
#define CAPTURE_MUTE_NUMID                  90
#define OPEN_MIC                            "1"
#define CLOSE_MIC                           "0"

/* params */
#define BUFFER_SIZE_DEFAULT                 9600
#define PERIOD_SIZE_DEFAULT                 2400
#define BUFFER_SIZE_CALL                    2560
#define PERIOD_SIZE_CALL                    1280
#define CAPTURE_BUFFER_SIZE_DEFAULT         2560
#define CAPTURE_PERIOD_SIZE_DEFAULT         640
#define CAPTURE_BUFFER_SIZE_CALL            320
#define CAPTURE_PERIOD_SIZE_CALL            160
#define ALSA_CAP_BUFFER_SIZE                (2 * 2 * (CAPTURE_PERIOD_SIZE_DEFAULT))
#define RATE_CALL                           8000
#define CHANNEL_CALL                        1

/* mute path */
#define MIXER_MUTE_PATH_NUM                 4
#define MIXER_MUTE_PATH_OFF                 "1 1024"
#define MIXER_MUTE_PATH_ON                  "0 1024"

/* DACL Playback Volume  */
#define     SND_NUMID_DACL_PLAYBACK_VOL      355
#define     SND_ELEM_DACL_PLAYBACK_VOL       "EAR Gain EAR Playback Volume"

/* DACR Playback Volume  */
#define     SND_NUMID_DACR_PLAYBACK_VOL      344
#define     SND_ELEM_DACR_PLAYBACK_VOL       "DAC Gain DAC Playback Volume"

/* DACL Capture Volume  */
#define     SND_NUMID_DACL_CAPTURE_VOL       340
#define     SND_ELEM_DACL_CAPTURE_VOL        "ADC_1 Gain ADC_1 Capture Volume"

/* DACR Capture Volume  */
#define     SND_NUMID_DACR_CAPTURE_VOL       341
#define     SND_ELEM_DACR_CAPTURE_VOL        "ADC_2 Gain ADC_2 Capture Volume"

/* voice volume */
#define     VOICE_VOLUME_PATH                "VBC_VOLUME"

/* sprd effect so path */
#define     SPRD_EFFECT_SO_PATH              "/vendor/lib64/libsprd_mock_effect.z.so"

#endif /* ALSA_SND_COMMON_H */
#endif /* COMMON_H */
