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

#include "alsa_snd_capture.h"
#include "common.h"

#define HDF_LOG_TAG HDF_AUDIO_HAL_CAPTURE
static enum AudioCategory g_currentScene = AUDIO_MMAP_NOIRQ;    // capture the current scene

typedef struct _CAPTURE_DATA_ {
    struct AlsaMixerCtlElement ctrlLeftVolume;
    struct AlsaMixerCtlElement ctrlRightVolume;
    long tempVolume;
}CaptureData;

static int32_t CaptureSetHwParamsImpl(struct AlsaCapture *captureIns, const struct AudioHwCaptureParam *handleData);
static int32_t CaptureInitImpl(struct AlsaCapture* captureIns);

static int32_t UpdateAudioCaptureRoute(struct AlsaCapture *captureIns, const struct AudioHwCaptureParam *handleData)
{
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)captureIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    int32_t ret;
    int32_t devCount = handleData->captureMode.hwInfo.pathSelect.deviceInfo.deviceNum;

    AUDIO_FUNC_LOGI("UpdateAudioCaptureRoute devCount:%{public}d!", devCount);
    if (devCount < 0 || devCount > PATHPLAN_COUNT - 1) {
        AUDIO_FUNC_LOGE("devCount is error!");
        return HDF_FAILURE;
    }

    struct AlsaMixerCtlElement elems[devCount];
    for (int i = 0; i < devCount; i++) {
        SndElementItemInit(&elems[i]);

        elems[i].numid = 0;
        elems[i].name = handleData->captureMode.hwInfo.pathSelect.deviceInfo.deviceSwitchs[i].deviceSwitch;
        elems[i].value = handleData->captureMode.hwInfo.pathSelect.deviceInfo.deviceSwitchs[i].value;
    }

    ret = SndElementGroupWrite(cardIns, elems, devCount);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("capture SndElementGroupWrite fail");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static void PcmCloseHandle(struct AlsaSoundCard *cardIns)
{
    AUDIO_FUNC_LOGE("capture PcmCloseHandle enter");
    int32_t ret;
    if (cardIns == NULL) {
        AUDIO_FUNC_LOGE("cardIns is NULL");
        return;
    }
    if (cardIns->cardStatus > 0) {
        cardIns->cardStatus -= 1;
    }
    if (cardIns->cardStatus == 0) {
        if (cardIns->pcmHandle != NULL) {
            ret = snd_pcm_close(cardIns->pcmHandle);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("snd_pcm_close fail: %{public}s", snd_strerror(ret));
            }
            cardIns->pcmHandle = NULL;
        }
        if (cardIns->mixerHandle != NULL) {
            ret = snd_mixer_close(cardIns->mixerHandle);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("mixer close error: %{public}s.", snd_strerror(ret));
            }
            cardIns->mixerHandle = NULL;
        }
    }
    AUDIO_FUNC_LOGE("capture PcmCloseHandle end");
}

static int32_t ReOpenPcmAndSetParams(struct AlsaCapture *captureIns, const struct AudioHwCaptureParam *handleData)
{
    AUDIO_FUNC_LOGE("capture ReOpenPcmAndSetParams enter.");
    int32_t ret;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)captureIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(handleData);

    enum AudioCategory scene = handleData->frameCaptureMode.attrs.type;
    uint32_t dev = CaptureGetSceneDev(scene);
    AUDIO_FUNC_LOGE("capture  ReOpenPcmAndSetParams scene: %{public}d, dev: %{public}d.", scene, dev);
    ret = SndSaveCardListInfo(SND_PCM_STREAM_PLAYBACK, dev);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("Failed to save card device info.");
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->captureMode.hwInfo.adapterName;
    ret = SndMatchSelAdapter(&captureIns->soundCard, adapterName);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SndMatchSelAdapter faild.");
        SndCloseHandle(&captureIns->soundCard);
        return HDF_FAILURE;
    }

    ret = snd_pcm_open(&cardIns->pcmHandle, cardIns->devName, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    AUDIO_FUNC_LOGI("capture snd_pcm_open devName: %{public}s!", cardIns->devName);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("snd_pcm_open fail: %{public}s!", snd_strerror(ret));
        SndCloseHandle(&captureIns->soundCard);
        return HDF_FAILURE;
    }
    ret = SndOpenMixer(&captureIns->soundCard);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SndOpenMixer failed");
        SndCloseHandle(&captureIns->soundCard);
        return HDF_FAILURE;
    }

    ret = CaptureInitImpl(captureIns);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams CaptureInitImpl fail");
        return HDF_FAILURE;
    }

    ret = UpdateAudioCaptureRoute(captureIns, handleData);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams UpdateAudioCaptureRoute fail");
        return HDF_FAILURE;
    }

    ret = CaptureSetHwParamsImpl(captureIns, handleData);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams CaptureSetHwParamsImpl fail");
        return HDF_FAILURE;
    }

    ret = snd_pcm_prepare(cardIns->pcmHandle);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("snd_pcm_prepare fail: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t CaptureInitImpl(struct AlsaCapture* captureIns)
{
    if (captureIns->priData != NULL) {
        return HDF_SUCCESS;
    }
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);

    CaptureData *priData = (CaptureData *)OsalMemCalloc(sizeof(CaptureData));
    if (priData == NULL) {
        AUDIO_FUNC_LOGE("Failed to allocate memory!");
        return HDF_FAILURE;
    }

    SndElementItemInit(&priData->ctrlLeftVolume);
    SndElementItemInit(&priData->ctrlRightVolume);
    priData->ctrlLeftVolume.numid = SND_NUMID_DACL_CAPTURE_VOL;
    priData->ctrlLeftVolume.name = SND_ELEM_DACL_CAPTURE_VOL;
    priData->ctrlRightVolume.numid = SND_NUMID_DACR_CAPTURE_VOL;
    priData->ctrlRightVolume.name = SND_ELEM_DACR_CAPTURE_VOL;
    CaptureSetPriData(captureIns, (CapturePriData)priData);

    return HDF_SUCCESS;
}

static bool CheckSceneIsChange(enum AudioCategory scene)
{
    if (scene != AUDIO_IN_CALL) {
        if (g_currentScene == AUDIO_IN_CALL || g_currentScene == AUDIO_MMAP_NOIRQ) {
            return true;
        } else {
            return false;
        }
    } else {
        if (g_currentScene != AUDIO_IN_CALL) {
            return true;
        } else {
            return false;
        }
    }
}

static int32_t CaptureSelectSceneImpl(struct AlsaCapture *captureIns, const struct AudioHwCaptureParam *handleData)
{
    CHECK_NULL_PTR_RETURN_DEFAULT(handleData);
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);
    int32_t ret = HDF_SUCCESS;
    AUDIO_FUNC_LOGI("CaptureSelectSceneImpl enter");
    captureIns->descPins = handleData->captureMode.hwInfo.deviceDescript.pins;
    enum AudioCategory scene = handleData->frameCaptureMode.attrs.type;

    if (CheckSceneIsChange(scene)) {
        AUDIO_FUNC_LOGI("CaptureSelectSceneImpl change scene");
        if (g_currentScene == AUDIO_MMAP_NOIRQ) {
            AUDIO_FUNC_LOGI("CaptureSelectSceneImpl scene is AUDIO_MMAP_NOIRQ");
            ret = UpdateAudioCaptureRoute(captureIns, handleData);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("capture UpdateAudioCaptureRoute fail");
                return HDF_FAILURE;
            }
            g_currentScene = scene;
        } else {
            AUDIO_FUNC_LOGI("CaptureSelectSceneImpl scene is not AUDIO_MMAP_NOIRQ scene: %{public}d", scene);
            if (captureIns->soundCard.pcmHandle != NULL) {
                snd_pcm_drop(captureIns->soundCard.pcmHandle);
            }
            PcmCloseHandle(&captureIns->soundCard);
            if (!SndisBusy(&captureIns->soundCard)) {
                AUDIO_FUNC_LOGI("CaptureSelectSceneImpl pcm is NULL");
                g_currentScene = scene;
            }
        }
    } else {
        g_currentScene = scene;
        if (scene == AUDIO_IN_CALL) {
            AUDIO_FUNC_LOGI("CaptureSelectSceneImpl change device");
            ret = UpdateAudioCaptureRoute(captureIns, handleData);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("capture UpdateAudioCaptureRoute fail");
                return HDF_FAILURE;
            }
        }
    }
    
    AUDIO_FUNC_LOGI("CaptureSelectSceneImpl end");

    return HDF_SUCCESS;
}

static int32_t CaptureGetVolThresholdImpl(struct AlsaCapture *captureIns, long *volMin, long *volMax)
{
    int32_t ret;
    long volMinVal = 0;
    long volMaxVal = 0;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)captureIns;
    CaptureData *priData = CaptureGetPriData(captureIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(priData);

    ret = SndElementReadRange(cardIns, &priData->ctrlLeftVolume, &volMinVal, &volMaxVal);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SndElementReadRange fail!");
        return HDF_FAILURE;
    }
    *volMin = volMinVal;
    *volMax = volMaxVal;
    
    return HDF_SUCCESS;
}

static int32_t CaptureGetVolumeImpl(struct AlsaCapture *captureIns, long *volume)
{
    int32_t ret;
    long volLeft = 0;
    long volRight = 0;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)captureIns;
    CaptureData *priData = CaptureGetPriData(captureIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(priData);

    ret = SndElementReadInt(cardIns, &priData->ctrlLeftVolume, &volLeft);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("Read left volume fail!");
        return HDF_FAILURE;
    }
    ret = SndElementReadInt(cardIns, &priData->ctrlRightVolume, &volRight);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("Read right volume fail!");
        return HDF_FAILURE;
    }
    *volume = (volLeft + volRight) >> 1;
    
    return HDF_SUCCESS;
}

static int32_t CaptureSetVolumeImpl(struct AlsaCapture *captureIns, long volume)
{
    int32_t ret;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)captureIns;
    CaptureData *priData = CaptureGetPriData(captureIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(priData);
    ret = SndElementWriteInt(cardIns, &priData->ctrlLeftVolume, volume);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("Write left volume fail!");
        return HDF_FAILURE;
    }
    ret = SndElementWriteInt(cardIns, &priData->ctrlRightVolume, volume);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("Write right volume fail!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t CaptureSetMuteImpl(struct AlsaCapture *captureIns, bool muteFlag)
{
    int32_t ret;

    CaptureData *priData = CaptureGetPriData(captureIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(priData);
    
    struct AlsaMixerCtlElement elem;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)captureIns;

    SndElementItemInit(&elem);
    elem.numid = CAPTURE_MUTE_NUMID;
    elem.name = CAPTURE_MUTE_PATH;
    elem.value = muteFlag ? OPEN_MIC : CLOSE_MIC;

    ret = SndElementWrite(cardIns, &elem);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("write capture fail!");
        return HDF_FAILURE;
    }

    captureIns->muteState = muteFlag;
    return HDF_SUCCESS;
}

static int32_t CaptureStartImpl(struct AlsaCapture *captureIns, const struct AudioHwCaptureParam *handleData)
{
    AUDIO_FUNC_LOGI("CaptureStartImpl enter!");

    uint32_t ret;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)captureIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);

    if (g_currentScene == AUDIO_IN_CALL) {
        if (cardIns->pcmHandle) {
            ret = snd_pcm_start(cardIns->pcmHandle);
            AUDIO_FUNC_LOGI("CaptureStartImpl use snd_pcm_start ret:%{public}d", ret);
            AUDIO_FUNC_LOGI("CaptureStartImpl  call  cardIns->devName: %{public}s!", cardIns->devName);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("snd_pcm_start fail: %{public}s, ret: %{public}d", snd_strerror(ret), ret);
            }

            return HDF_SUCCESS;
        }
        return HDF_FAILURE;
    } else {
        if (!SndisBusy(cardIns)) {
            AUDIO_FUNC_LOGI("CaptureStartImpl pcmHandle is NULL");
            ret = ReOpenPcmAndSetParams(captureIns, handleData);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams fail");
                return HDF_FAILURE;
            }
        }
    }

    AUDIO_FUNC_LOGI("CaptureStartImpl end!");
    return HDF_SUCCESS;
}

static int32_t CaptureStopImpl(struct AlsaCapture *captureIns)
{
    AUDIO_FUNC_LOGI("CaptureStopImpl enter!");
    if (g_currentScene == AUDIO_IN_CALL) {
        return HDF_SUCCESS;
    }
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns->soundCard.pcmHandle);

    snd_pcm_drop(captureIns->soundCard.pcmHandle);
    PcmCloseHandle(&captureIns->soundCard);

    AUDIO_FUNC_LOGI("CaptureStopImpl end!");
    return HDF_SUCCESS;
}

static int32_t CaptureGetGainThresholdImpl(struct AlsaCapture *captureIns, float *gainMin, float *gainMax)
{
    AUDIO_FUNC_LOGE("not support gain operation");
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);
    (void)gainMin;
    (void)gainMax;
    return HDF_SUCCESS;
}

static int32_t CaptureGetGainImpl(struct AlsaCapture *captureIns, float *volume)
{
    AUDIO_FUNC_LOGE("not support gain operation");
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);
    (void)volume;
    return HDF_SUCCESS;
}

static int32_t CaptureSetGainImpl(struct AlsaCapture *captureIns, float volume)
{
    AUDIO_FUNC_LOGE("not support gain operation");
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);
    (void)volume;
    return HDF_SUCCESS;
}

static bool CaptureGetMuteImpl(struct AlsaCapture *captureIns)
{
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);
    return captureIns->muteState;
}

static int32_t CaptureDataCopyVdi(struct AudioHwCaptureParam *handleData, char *buffer, uint64_t frames)
{
    int32_t ret;
    uint32_t channels;
    uint32_t format;
    uint64_t recvDataSize;
    CHECK_NULL_PTR_RETURN_DEFAULT(handleData);
    CHECK_NULL_PTR_RETURN_DEFAULT(buffer);
    if (frames == 0) {
        AUDIO_FUNC_LOGE("Capture buffer size is empty!");
        return HDF_FAILURE;
    }

    if (handleData->frameCaptureMode.buffer == NULL) {
        AUDIO_FUNC_LOGE("frameCaptureMode.buffer is NULL!");
        return HDF_FAILURE;
    }
    channels = handleData->frameCaptureMode.attrs.channelCount;
    format = (uint32_t)handleData->frameCaptureMode.attrs.format;
    recvDataSize = (uint64_t)(frames * channels * format);
    ret = memcpy_s(handleData->frameCaptureMode.buffer, FRAME_DATA, buffer, recvDataSize);
    if (ret != EOK) {
        AUDIO_FUNC_LOGE("memcpy frame data failed!");
        return HDF_FAILURE;
    }
    handleData->frameCaptureMode.bufferSize = recvDataSize;
    handleData->frameCaptureMode.bufferFrameSize = frames;

    return HDF_SUCCESS;
}

static int32_t CapturePcmReadiVdi(snd_pcm_t *pcm, uint64_t *frameCnt, char *dataBuf, snd_pcm_uframes_t bufSize)
{
    int32_t ret;
    long frames;
    int32_t tryNum = AUDIO_ALSALIB_RETYR;
    CHECK_NULL_PTR_RETURN_DEFAULT(pcm);
    CHECK_NULL_PTR_RETURN_DEFAULT(frameCnt);
    CHECK_NULL_PTR_RETURN_DEFAULT(dataBuf);
    if (bufSize == 0) {
        AUDIO_FUNC_LOGE("Capture data buf is empty.");
        return HDF_FAILURE;
    }

    while (bufSize > 0) {
        /* Read interleaved frames to a PCM. */
        AUDIO_FUNC_LOGI("snd_pcm_mmap_readi begin bufSize: %{public}ld", bufSize);
        frames = snd_pcm_mmap_readi(pcm, dataBuf, bufSize);
        AUDIO_FUNC_LOGI("snd_pcm_mmap_readi end frames: %{public}ld", frames);
        if (frames > 0) {
            *frameCnt = (uint64_t)frames;
            bufSize -= frames;
            return HDF_SUCCESS;
        }

        if (frames == -EBADFD) {
            AUDIO_FUNC_LOGE("Capture PCM is not in the right state: %{public}s", snd_strerror(frames));
            ret = snd_pcm_prepare(pcm);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("Capture snd_pcm_prepare fail: %{public}s", snd_strerror(ret));
                return HDF_FAILURE;
            }
        } else if (frames == -EAGAIN) {
            AUDIO_FUNC_LOGE("snd_pcm_mmap_writei faild snd_pcm_wait");
            snd_pcm_wait(pcm, PCM_WAIT_TIMEOUT_MS);
            tryNum--;
            if (tryNum == 0) {
                return HDF_SUCCESS;
            }
        } else {
            /* -ESTRPIPE: a suspend event occurred,
             * stream is suspended and waiting for an application recovery.
             * -EPIPE: an underrun occurred.
             */
            ret = snd_pcm_recover(pcm, frames, 0); // 0 for open capture recover log.
            if (ret < 0) {
                AUDIO_FUNC_LOGE("snd_pcm_mmap_readi failed: %{public}s", snd_strerror(ret));
                return HDF_FAILURE;
            }
        }
    }

    return HDF_SUCCESS;
}

static int32_t CheckPcmStatus(struct AlsaCapture *captureIns, struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);
    ret = snd_pcm_wait(captureIns->soundCard.pcmHandle, PCM_WAIT_TIMEOUT_MS); /* -1 for timeout, Waiting forever */
    if (ret < 0) {
        AUDIO_FUNC_LOGE("snd_pcm_wait failed: %{public}s.", snd_strerror(ret));

        if (captureIns->soundCard.pcmHandle != NULL) {
            snd_pcm_drop(captureIns->soundCard.pcmHandle);
        }
        PcmCloseHandle(&captureIns->soundCard);
        if (!SndisBusy(&captureIns->soundCard)) {
            AUDIO_FUNC_LOGI("CaptureSelectSceneImpl pcm is NULL");
            ret = ReOpenPcmAndSetParams(captureIns, handleData);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams fail");
                return HDF_FAILURE;
            }
        }

        return HDF_FAILURE;
    }

    if (snd_pcm_state(captureIns->soundCard.pcmHandle) == SND_PCM_STATE_SETUP) {
        ret = snd_pcm_prepare(captureIns->soundCard.pcmHandle);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("snd_pcm_prepare fail: %{public}s", snd_strerror(ret));
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static int32_t CheckCapFrameBufferSize(struct AudioHwCaptureParam *handleData, snd_pcm_uframes_t *periodSize)
{
    uint32_t capFrameSize;
    uint64_t capReqBufferSize;
    CHECK_NULL_PTR_RETURN_DEFAULT(handleData);
    CHECK_NULL_PTR_RETURN_DEFAULT(periodSize);

    capFrameSize = handleData->frameCaptureMode.attrs.channelCount * handleData->frameCaptureMode.attrs.format;
    if (capFrameSize == 0) {
        AUDIO_FUNC_LOGE("Capture frame size is zero!");
        return HDF_FAILURE;
    }
    capReqBufferSize = capFrameSize * (*periodSize);
    if (capReqBufferSize > FRAME_DATA) {
        *periodSize = FRAME_DATA / capFrameSize;
    }

    return HDF_SUCCESS;
}

static int32_t CaptureReadImpl(struct AlsaCapture *captureIns, struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    uint64_t frames = 0;
    char *buffer = NULL;
    snd_pcm_uframes_t bufferSize = 0;
    snd_pcm_uframes_t periodSize = 0;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)captureIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(handleData);

    if (cardIns->pauseState) {
        AUDIO_FUNC_LOGE("Currently in pause, please check!");
        return HDF_FAILURE;
    }
    ret = snd_pcm_get_params(cardIns->pcmHandle, &bufferSize, &periodSize);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Get capture params error: %{public}s.", snd_strerror(ret));
        return HDF_FAILURE;
    }
    if (CheckCapFrameBufferSize(handleData, &periodSize) != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("CheckCapFrameBufferSize failed.");
        return HDF_FAILURE;
    }
    if (CheckPcmStatus(captureIns, handleData) != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("CheckPcmStatus failed.");
        return HDF_FAILURE;
    }
    buffer = OsalMemCalloc(ALSA_CAP_BUFFER_SIZE);
    if (buffer == NULL) {
        AUDIO_FUNC_LOGE("Failed to Calloc buffer");
        return HDF_FAILURE;
    }
    ret = CapturePcmReadiVdi(cardIns->pcmHandle, &frames, buffer, periodSize);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("CapturePcmReadiVdi is error!");
        AudioMemFree((void **)&buffer);
        return ret;
    }
    ret = CaptureDataCopyVdi(handleData, buffer, frames);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("Failed to copy data. It may be paused. Check the status!");
        AudioMemFree((void **)&buffer);
        return ret;
    }

    AudioMemFree((void **)&buffer);
    return HDF_SUCCESS;
}

static int32_t SaveHwParams(struct AlsaSoundCard *cardIns, const struct AudioHwCaptureParam *handleData)
{
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(handleData);

    cardIns->hwParams.streamType = AUDIO_CAPTURE_STREAM;
    cardIns->hwParams.channels = handleData->frameCaptureMode.attrs.channelCount;
    cardIns->hwParams.rate = handleData->frameCaptureMode.attrs.sampleRate;
    cardIns->hwParams.periodSize = handleData->frameCaptureMode.periodSize;
    cardIns->hwParams.periodCount = handleData->frameCaptureMode.periodCount;
    cardIns->hwParams.format = handleData->frameCaptureMode.attrs.format;
    cardIns->hwParams.period = handleData->frameCaptureMode.attrs.period;
    cardIns->hwParams.frameSize = handleData->frameCaptureMode.attrs.frameSize;
    cardIns->hwParams.isBigEndian = handleData->frameCaptureMode.attrs.isBigEndian;
    cardIns->hwParams.isSignedData = handleData->frameCaptureMode.attrs.isSignedData;
    cardIns->hwParams.startThreshold = handleData->frameCaptureMode.attrs.startThreshold;
    cardIns->hwParams.stopThreshold = handleData->frameCaptureMode.attrs.stopThreshold;
    cardIns->hwParams.silenceThreshold = handleData->frameCaptureMode.attrs.silenceThreshold;

    return HDF_SUCCESS;
}

static int32_t SetHWParamsCall(struct AlsaSoundCard *cardIns)
{
    AUDIO_FUNC_LOGI("SetHWParamsCall begin.");
    int32_t ret;
    uint32_t channel = CHANNEL_CALL;
    uint32_t rRate = RATE_CALL;
    int32_t dir = 0;
    snd_pcm_format_t pcmFormat = SND_PCM_FORMAT_S16_LE;
    snd_pcm_uframes_t size = 0;
    snd_pcm_hw_params_t *hwParams = NULL;
    snd_pcm_t *handle = cardIns->pcmHandle;
    struct AlsaCapture *captureIns = (struct AlsaCapture*)cardIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(handle);

    snd_pcm_hw_params_alloca(&hwParams);
    ret = snd_pcm_hw_params_any(handle, hwParams); // choose all parameters
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Broken configuration for capture: %{public}s.", snd_strerror(ret));
        return HDF_FAILURE;
    }

    snd_pcm_access_mask_t *mask = malloc(snd_pcm_access_mask_sizeof());
    snd_pcm_access_mask_none(mask);
    snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_INTERLEAVED);
    snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_NONINTERLEAVED);
    snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_COMPLEX);
    ret = snd_pcm_hw_params_set_access_mask(handle, hwParams, mask);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("snd_pcm_hw_params_set_access_mask failed: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    ret = snd_pcm_hw_params_set_format(handle, hwParams, pcmFormat);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Sample format not available for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    ret = snd_pcm_hw_params_set_channels(handle, hwParams, channel);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Channels count (%{public}d) not available for capture: %{public}s", ret, snd_strerror(ret));
        return HDF_FAILURE;
    }

    ret = snd_pcm_hw_params_set_rate_near(handle, hwParams, &rRate, &dir);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("snd_pcm_hw_params_set_rate_near failed!");
        return ret;
    }

    captureIns->periodSize = CAPTURE_PERIOD_SIZE_CALL;
    captureIns->bufferSize = CAPTURE_BUFFER_SIZE_CALL;
    size = captureIns->periodSize;
    ret = snd_pcm_hw_params_set_period_size_near(handle, hwParams, &size, 0);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SetHWParamsCall Set period size failed!");
        return ret;
    }
    AUDIO_FUNC_LOGI("SetHWParamsCall use snd_pcm_hw_params_set_period_size_near");

    size = captureIns->bufferSize;
    ret = snd_pcm_hw_params_set_buffer_size_near(handle, hwParams, &size);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SetHWParamsCall Set buffer size failed!");
        return ret;
    }

    ret = snd_pcm_hw_params(handle, hwParams);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("SetHWParamsCall Unable to set hw params for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t SetHWParamsSubVdi(
    struct AlsaSoundCard *cardIns, snd_pcm_hw_params_t *params)
{
    int32_t ret;
    snd_pcm_format_t pcmFormat;
    snd_pcm_t *handle = cardIns->pcmHandle;
    struct AlsaCapture *captureIns = (struct AlsaCapture *)cardIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(params);

    ret = snd_pcm_hw_params_set_rate_resample(handle, params, captureIns->resample);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Resampling setup failed for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    snd_pcm_access_mask_t *mask = malloc(snd_pcm_access_mask_sizeof());
    snd_pcm_access_mask_none(mask);
    snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_INTERLEAVED);
    snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_NONINTERLEAVED);
    snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_COMPLEX);
    ret = snd_pcm_hw_params_set_access_mask(handle, params, mask);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Access type not available for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }
    ret = SndConverAlsaPcmFormat(&cardIns->hwParams, &pcmFormat);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SndConverAlsaPcmFormat error.");
        return ret;
    }

    ret = snd_pcm_hw_params_set_format(handle, params, pcmFormat);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Sample format not available for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    AUDIO_FUNC_LOGI("SetHWParamsSubVdi channels:%{public}d", cardIns->hwParams.channels);
    ret = snd_pcm_hw_params_set_channels(handle, params, CHANNEL_CALL);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Channels count (%{public}u) not available for capture: %{public}s", cardIns->hwParams.channels,
            snd_strerror(ret));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t SetHWRate(struct AlsaSoundCard *cardIns, snd_pcm_hw_params_t *params)
{
    int32_t ret;
    uint32_t rRate;
    int32_t dir = 0; /* dir Value range (-1,0,1) */
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(params);

    /* set the stream rate */
    rRate = cardIns->hwParams.rate;
    AUDIO_FUNC_LOGI("SetHWRate first rRate: %{public}du", rRate);
    ret = snd_pcm_hw_params_set_rate_near(cardIns->pcmHandle, params, &rRate, &dir);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Rate %{public}uHz not available for capture: %{public}s",
            cardIns->hwParams.rate, snd_strerror(ret));
        return HDF_FAILURE;
    }

    if (rRate != cardIns->hwParams.rate) {
        ret = snd_pcm_hw_params_set_rate_near(cardIns->pcmHandle, params, &rRate, &dir);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("Rate %{public}uHz not available for capture: %{public}s",
                cardIns->hwParams.rate, snd_strerror(ret));
            return HDF_FAILURE;
        }
    }
    /* Update to hardware supported rate */
    cardIns->hwParams.rate = rRate;
    AUDIO_FUNC_LOGI("SetHWRate sconed rRate: %{public}du", rRate);

    return HDF_SUCCESS;
}

static int32_t SetHWBufferAndPeriod(struct AlsaSoundCard *cardIns, snd_pcm_hw_params_t *params)
{
    int32_t ret;
    snd_pcm_uframes_t size = 0;
    struct AlsaCapture *captureIns = (struct AlsaCapture*)cardIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(params);

    captureIns->periodSize = CAPTURE_PERIOD_SIZE_DEFAULT;
    captureIns->bufferSize = CAPTURE_BUFFER_SIZE_DEFAULT;
    size = captureIns->periodSize;
    ret = snd_pcm_hw_params_set_period_size_near(cardIns->pcmHandle, params, &size, 0);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SetHWParamsCall Set period size failed!");
        return ret;
    }

    size = captureIns->bufferSize;
    ret = snd_pcm_hw_params_set_buffer_size_near(cardIns->pcmHandle, params, &size);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SetHWParamsCall Set buffer size failed!");
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t SetHWParamsVdi(struct AlsaSoundCard *cardIns)
{
    AUDIO_FUNC_LOGI("SetHWParamsVdi enter!");
    int32_t ret;
    snd_pcm_hw_params_t *hwParams = NULL;
    snd_pcm_t *handle = cardIns->pcmHandle;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(handle);

    snd_pcm_hw_params_alloca(&hwParams);
    ret = snd_pcm_hw_params_any(handle, hwParams); // choose all parameters
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Broken configuration for capture: no configurations available: %{public}s.",
            snd_strerror(ret));
        return HDF_FAILURE;
    }
    ret = SetHWParamsSubVdi(cardIns, hwParams);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SetHWParamsSubVdi failed!");
        return ret;
    }
    ret = SetHWRate(cardIns, hwParams);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SetHWRate failed!");
        return ret;
    }
    AUDIO_FUNC_LOGI("SetHWParams SetHWBufferAndPeriod begin!");
    ret = SetHWBufferAndPeriod(cardIns, hwParams);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SetHWBuffer failed!");
        return ret;
    }
    AUDIO_FUNC_LOGI("SetHWParamsVdi snd_pcm_hw_params begin!");
    ret = snd_pcm_hw_params(handle, hwParams);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Unable to set hw params for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }
    AUDIO_FUNC_LOGI("SetHWParamsVdi snd_pcm_hw_params end!");
    cardIns->canPause = snd_pcm_hw_params_can_pause(hwParams);
    AUDIO_FUNC_LOGI("hardware driver %{public}s pause", cardIns->canPause ? "supports" : "does not support");

    return HDF_SUCCESS;
}

static int32_t SetSWParams(struct AlsaSoundCard *cardIns)
{
    int32_t ret;
    /* The time when the application starts reading data */
    snd_pcm_sframes_t startThresholdSize = 1;
    snd_pcm_sframes_t stopThresholdSize = -1;
    snd_pcm_sw_params_t *swParams = NULL;
    snd_pcm_t *handle = cardIns->pcmHandle;
    struct AlsaCapture *captureIns = (struct AlsaCapture *)cardIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(handle);

    snd_pcm_sw_params_alloca(&swParams);
    /* get the current swparams */
    ret = snd_pcm_sw_params_current(handle, swParams);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Unable to determine current swparams for capture: %{public}s.", snd_strerror(ret));
        return HDF_FAILURE;
    }
    if (captureIns->periodSize == 0) {
        AUDIO_FUNC_LOGE("error: g_periodSize cannot be zero!");
        return HDF_FAILURE;
    }

    if (g_currentScene == AUDIO_IN_CALL) {
        ret = snd_pcm_sw_params_set_avail_min(handle, swParams, 0);
        ret = snd_pcm_sw_params_set_start_threshold(handle, swParams, 0);
        ret = snd_pcm_sw_params_set_silence_size(handle, swParams, 0);
        ret = snd_pcm_sw_params_set_silence_threshold(handle, swParams, 0);
        ret = snd_pcm_sw_params_set_stop_threshold(handle, swParams, stopThresholdSize);
    } else {
        /* start the transfer when the buffer is 1 frames */
        ret = snd_pcm_sw_params_set_start_threshold(handle, swParams, startThresholdSize);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("Unable to set start threshold mode for capture: %{public}s.", snd_strerror(ret));
            return HDF_FAILURE;
        }

        /* allow the transfer when at least period_size samples can be processed */
        /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
        ret = snd_pcm_sw_params_set_avail_min(handle, swParams,
            captureIns->periodEvent ? captureIns->bufferSize : captureIns->periodSize);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("Unable to set avail min for capture: %{public}s", snd_strerror(ret));
            return HDF_FAILURE;
        }
    }

    /* enable period events when requested */
    if (captureIns->periodEvent) {
        ret = snd_pcm_sw_params_set_period_event(handle, swParams, 1);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("Unable to set period event: %{public}s", snd_strerror(ret));
            return HDF_FAILURE;
        }
    }
    /* write the parameters to the capture device */
    ret = snd_pcm_sw_params(handle, swParams);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Unable to set sw params for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t CaptureSetParams(struct AlsaCapture *captureIns, const struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)captureIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(handleData);

    SaveHwParams(cardIns, handleData);

    if (g_currentScene != AUDIO_IN_CALL) {
        ret = SetHWParamsVdi(cardIns);
    } else {
        ret = SetHWParamsCall(cardIns);
    }

    if (ret < 0) {
        AUDIO_FUNC_LOGE("Setting of hwparams failed.");
        return HDF_FAILURE;
    }

    ret = SetSWParams(cardIns);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Setting of swparams failed.");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t CaptureSetHwParamsImpl(struct AlsaCapture *captureIns, const struct AudioHwCaptureParam *handleData)
{
    int32_t ret;
    CHECK_NULL_PTR_RETURN_DEFAULT(captureIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(handleData);
    ret = CaptureSetParams(captureIns, handleData);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("capture set parameters failed!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t CaptureOverrideFunc(struct AlsaCapture *captureIns)
{
    if (captureIns == NULL) {
        return HDF_FAILURE;
    }
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)captureIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);

    if (cardIns->cardType == SND_CARD_PRIMARY) {
        captureIns->Init = CaptureInitImpl;
        captureIns->SelectScene = CaptureSelectSceneImpl;
        captureIns->Start = CaptureStartImpl;
        captureIns->Stop = CaptureStopImpl;
        captureIns->GetVolThreshold = CaptureGetVolThresholdImpl;
        captureIns->GetVolume = CaptureGetVolumeImpl;
        captureIns->SetVolume = CaptureSetVolumeImpl;
        captureIns->GetGainThreshold = CaptureGetGainThresholdImpl;
        captureIns->GetGain = CaptureGetGainImpl;
        captureIns->SetGain = CaptureSetGainImpl;
        captureIns->GetMute = CaptureGetMuteImpl;
        captureIns->SetMute = CaptureSetMuteImpl;

        captureIns->Read = CaptureReadImpl;
        captureIns->SetParams = CaptureSetHwParamsImpl;
    }
    
    return HDF_SUCCESS;
}

int32_t CaptureGetSceneDev(enum AudioCategory scene)
{
    if (scene < AUDIO_IN_MEDIA || scene > AUDIO_MMAP_NOIRQ) {
        scene = AUDIO_IN_MEDIA;
    }
    if (scene == AUDIO_IN_CALL) {
        return SND_CALL_PCM_DEV;
    } else {
        return SND_DEFAULT_PCM_DEV;
    }
}
