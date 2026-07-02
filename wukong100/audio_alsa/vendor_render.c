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

#include "alsa_snd_render.h"
#include "common.h"

#define HDF_LOG_TAG HDF_AUDIO_HAL_RENDER

static enum AudioCategory g_currentScene = AUDIO_MMAP_NOIRQ; // render the current scene
void *g_dlHandle;
typedef int32_t (*InitFunc)(const char *);
typedef int32_t (*UploadFunc)(void);
typedef int32_t (*UpdateFunc)(int);
typedef void (*CloseFunc)(void);
static snd_pcm_t *CaptureHandle = NULL;

static int32_t RenderSetHwParamsImpl(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData);
static int32_t SetHWParamsCapture(snd_pcm_t *pcmHandle);
static int32_t RenderInitImpl(struct AlsaRender *renderIns);
static int32_t RenderSetVoiceVolumeImpl(struct AlsaRender *renderIns, float volume);
static int32_t RenderStartImpl(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData);

typedef struct _RENDER_DATA_ {
    struct AlsaMixerCtlElement ctrlLeftVolume;
    struct AlsaMixerCtlElement ctrlRightVolume;
    long tempVolume;
}RenderData;

static const char* g_mixerMutePaths[MIXER_MUTE_PATH_NUM] = {
    "VBC DAC0 AUD MDG Set",
    "VBC DAC0 DSP MDG Set",
    "VBC DAC1 DSP MDG Set",
    "VBC DAC0 AUD23 MDG Set"
};

static int32_t AudioMixerSetMuteOff(struct AlsaRender *renderIns)
{
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    int32_t ret;
    struct AlsaMixerCtlElement elems[MIXER_MUTE_PATH_NUM];
    for (int i = 0; i < MIXER_MUTE_PATH_NUM; i++) {
        SndElementItemInit(&elems[i]);

        elems[i].numid = 0;
        elems[i].name = g_mixerMutePaths[i];
        elems[i].value = MIXER_MUTE_PATH_OFF;
    }
    ret = SndElementGroupWrite(cardIns, elems, MIXER_MUTE_PATH_NUM);
    if (ret < 0) {
        AUDIO_FUNC_LOGI("render SndElementGroupWrite fail");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t AudioMixerSetMuteOn(struct AlsaRender *renderIns)
{
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    int32_t ret;
    struct AlsaMixerCtlElement elems[MIXER_MUTE_PATH_NUM];
    for (int i = 0; i < MIXER_MUTE_PATH_NUM; i++) {
        SndElementItemInit(&elems[i]);

        elems[i].numid = 0;
        elems[i].name = g_mixerMutePaths[i];
        elems[i].value = MIXER_MUTE_PATH_ON;
    }
    ret = SndElementGroupWrite(cardIns, elems, MIXER_MUTE_PATH_NUM);
    if (ret < 0) {
        AUDIO_FUNC_LOGI("render SndElementGroupWrite fail");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t UpdateAudioRenderRoute(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData)
{
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    int32_t ret;
    int32_t devCount = handleData->renderMode.hwInfo.pathSelect.deviceInfo.deviceNum;

    AUDIO_FUNC_LOGI("UpdateAudioRenderRoute devCount:%{public}d!", devCount);
    if (devCount <= 0 || devCount > PATHPLAN_COUNT) {
        AUDIO_FUNC_LOGE("devCount is error!");
        return HDF_FAILURE;
    }

    struct AlsaMixerCtlElement elems[devCount];
    for (int i = 0; i < devCount; i++) {
        SndElementItemInit(&elems[i]);

        elems[i].numid = 0;
        elems[i].name = handleData->renderMode.hwInfo.pathSelect.deviceInfo.deviceSwitchs[i].deviceSwitch;
        elems[i].value = handleData->renderMode.hwInfo.pathSelect.deviceInfo.deviceSwitchs[i].value;
    }

    ret = SndElementGroupWrite(cardIns, elems, devCount);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("render SndElementGroupWrite fail");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static void PcmCloseHandle(struct AlsaSoundCard *cardIns)
{
    AUDIO_FUNC_LOGE("render PcmCloseHandle enter");
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

    if (CaptureHandle != NULL) {
        snd_pcm_drop(CaptureHandle);
        snd_pcm_close(CaptureHandle);
        CaptureHandle = NULL;
        AUDIO_FUNC_LOGE("capture PcmCloseHandle end");
    }
    AUDIO_FUNC_LOGE("render PcmCloseHandle end");
}

static int32_t ConfigurePcm(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData,
    struct AlsaSoundCard *cardIns)
{
    int32_t ret = UpdateAudioRenderRoute(renderIns, handleData);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams UpdateAudioRenderRoute fail");
        return HDF_FAILURE;
    }
    ret = RenderSetHwParamsImpl(renderIns, handleData);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams RenderSetHwParamsImpl fail");
        return HDF_FAILURE;
    }
    ret = snd_pcm_prepare(cardIns->pcmHandle);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("snd_pcm_prepare fail: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t ReOpenPcmAndSetParams(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData)
{
    AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams enter.");
    int32_t ret;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(handleData);

    enum AudioCategory scene = handleData->frameRenderMode.attrs.type;
    uint32_t dev = RenderGetSceneDev(scene);
    AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams scene: %{public}d, dev: %{public}d.", scene, dev);
    ret = SndSaveCardListInfo(SND_PCM_STREAM_PLAYBACK, dev);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("Failed to save card device info.");
        return HDF_FAILURE;
    }

    const char *adapterName = handleData->renderMode.hwInfo.adapterName;
    ret = SndMatchSelAdapter(&renderIns->soundCard, adapterName);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SndMatchSelAdapter faild.");
        SndCloseHandle(&renderIns->soundCard);
        return HDF_FAILURE;
    }

    ret = snd_pcm_open(&cardIns->pcmHandle, cardIns->devName, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    AUDIO_FUNC_LOGI("snd_pcm_open devName: %{public}s!", cardIns->devName);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("snd_pcm_open fail: %{public}s!", snd_strerror(ret));
        SndCloseHandle(&renderIns->soundCard);
        return HDF_FAILURE;
    }
    ret = SndOpenMixer(&renderIns->soundCard);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SndOpenMixer failed");
        SndCloseHandle(&renderIns->soundCard);
        return HDF_FAILURE;
    }

    ret = RenderInitImpl(renderIns);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams RenderInitImpl fail");
        return HDF_FAILURE;
    }

    ret = ConfigurePcm(renderIns, handleData, cardIns);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams ConfigurePcm fail");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t OpenCapturePcmAndSetParams()
{
    int32_t ret;
    AUDIO_FUNC_LOGI("snd_pcm_open capture hw:0,5 !");
    ret = snd_pcm_open(&CaptureHandle, SND_CALL_CAPTURE_DEV, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Open Capture hw:0,5 fail");
        return HDF_FAILURE;
    }

    ret = SetHWParamsCapture(CaptureHandle);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("SetHWParamsCapture fail");
        return HDF_FAILURE;
    }

    ret = snd_pcm_prepare(CaptureHandle);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("snd_pcm_prepare fail: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RenderInitImpl(struct AlsaRender *renderIns)
{
    AUDIO_FUNC_LOGI("RenderInitImpl enter!");
    if (renderIns->priData != NULL) {
        return HDF_SUCCESS;
    }
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);

    RenderData *priData = (RenderData *)OsalMemCalloc(sizeof(RenderData));
    if (priData == NULL) {
        AUDIO_FUNC_LOGE("Failed to allocate memory!");
        return HDF_FAILURE;
    }
    AUDIO_FUNC_LOGI("RenderInitImpl set volume!");
    SndElementItemInit(&priData->ctrlLeftVolume);
    SndElementItemInit(&priData->ctrlRightVolume);
    priData->ctrlLeftVolume.numid = SND_NUMID_DACL_PLAYBACK_VOL;
    priData->ctrlLeftVolume.name = SND_ELEM_DACL_PLAYBACK_VOL;
    priData->ctrlRightVolume.numid = SND_NUMID_DACR_PLAYBACK_VOL;
    priData->ctrlRightVolume.name = SND_ELEM_DACR_PLAYBACK_VOL;
    RenderSetPriData(renderIns, (RenderPriData)priData);

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

static int32_t ChangeScene(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData,
    enum AudioCategory scene)
{
    int32_t ret = HDF_SUCCESS;
    
    AUDIO_FUNC_LOGI("RenderSelectSceneImpl scene is not AUDIO_MMAP_NOIRQ");
    if (renderIns->soundCard.pcmHandle != NULL) {
        if (g_currentScene != AUDIO_IN_CALL) {
            snd_pcm_drain(renderIns->soundCard.pcmHandle);
        } else {
            snd_pcm_drop(renderIns->soundCard.pcmHandle);
        }
    }
    PcmCloseHandle(&renderIns->soundCard);
    if (!SndisBusy(&renderIns->soundCard)) {
        AUDIO_FUNC_LOGI("RenderSelectSceneImpl pcm is NULL");
        AUDIO_FUNC_LOGI("ChangeScene g_currentScene: %{public}d", g_currentScene);
        AUDIO_FUNC_LOGI("ChangeScene scene: %{public}d", scene);
        g_currentScene = scene;
        if (scene == AUDIO_IN_CALL) {
            if (AudioMixerSetMuteOff(renderIns) < 0) {
                AUDIO_FUNC_LOGE("AudioMixerSetMuteOff fail");
                return HDF_FAILURE;
            }
            ret = ReOpenPcmAndSetParams(renderIns, handleData);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams fail");
                return HDF_FAILURE;
            }
            ret = OpenCapturePcmAndSetParams();
            if (ret < 0) {
                AUDIO_FUNC_LOGE("OpenCapturePcmAndSetParams fail");
                return HDF_FAILURE;
            }
            if (AudioMixerSetMuteOn(renderIns) < 0) {
                AUDIO_FUNC_LOGE("AudioMixerSetMuteOn fail");
                return HDF_FAILURE;
            }
        }
        RenderStartImpl(renderIns, handleData);
    }
    
    return HDF_SUCCESS;
}

static int32_t RenderSelectSceneImpl(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData)
{
    CHECK_NULL_PTR_RETURN_DEFAULT(handleData);
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    int32_t ret = HDF_SUCCESS;
    
    renderIns->descPins = handleData->renderMode.hwInfo.deviceDescript.pins;
    enum AudioCategory scene = handleData->frameRenderMode.attrs.type;
    AUDIO_FUNC_LOGI("RenderSelectSceneImpl enter scene: %{public}d, device: %{public}d", scene, renderIns->descPins);
    if (CheckSceneIsChange(scene)) {
        if (g_currentScene == AUDIO_MMAP_NOIRQ) {
            AUDIO_FUNC_LOGI("RenderSelectSceneImpl scene is AUDIO_MMAP_NOIRQ");
            ret = UpdateAudioRenderRoute(renderIns, handleData);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("render UpdateAudioRenderRoute fail");
                return HDF_FAILURE;
            }
            g_currentScene = scene;
        } else {
            if (ChangeScene(renderIns, handleData, scene) < 0) {
                AUDIO_FUNC_LOGE("ChangeScene fail");
                return HDF_FAILURE;
            }
        }
    } else {
        g_currentScene = scene;
        AUDIO_FUNC_LOGI("RenderSelectSceneImpl not need change scene");
        if (scene == AUDIO_IN_CALL) {
            AUDIO_FUNC_LOGI("RenderSelectSceneImpl change device");
            ret = UpdateAudioRenderRoute(renderIns, handleData);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("render UpdateAudioRenderRoute fail");
                return HDF_FAILURE;
            }
            if (!g_dlHandle) {
                AUDIO_FUNC_LOGE("open libsprd_mock_effect_lib.z.so failed!");
                return HDF_FAILURE;
            }
            UpdateFunc updateDevice = (UpdateFunc)dlsym(g_dlHandle, "update_device");
            if (updateDevice == NULL) {
                AUDIO_FUNC_LOGE("update_device not defined or exported!");
                return HDF_FAILURE;
            }
            ret = updateDevice(handleData->renderMode.hwInfo.deviceDescript.pins);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("render update_device failed");
                return HDF_FAILURE;
            }
        }
    }
    return HDF_SUCCESS;
}

static int32_t RenderGetVolThresholdImpl(struct AlsaRender *renderIns, long *volMin, long *volMax)
{
    int32_t ret;
    long volMinVal = 0;
    long volMaxVal = 0;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;
    RenderData *priData = RenderGetPriData(renderIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(priData);

    ret = SndElementReadRange(cardIns, &priData->ctrlLeftVolume, &volMaxVal, &volMaxVal);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SndElementReadRange fail!");
        return HDF_FAILURE;
    }
    *volMin = volMinVal;
    *volMax = volMaxVal;
    
    return HDF_SUCCESS;
}

static int32_t RenderGetVolumeImpl(struct AlsaRender *renderIns, long *volume)
{
    int32_t ret;
    long volLeft = 0;
    long volRight = 0;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;
    RenderData *priData = RenderGetPriData(renderIns);
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

static int32_t RenderSetVolumeImpl(struct AlsaRender *renderIns, long volume)
{
    int32_t ret;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;
    RenderData *priData = RenderGetPriData(renderIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(priData);

    float volumeHdi = (float)volume / AUDIO_VOLUME_TO_HDI_FLOAT_DIVISOR;
    AUDIO_FUNC_LOGE("RenderSetVolumeImpl volume: %{public}ld, volumeHdi: %{public}f!", volume, volumeHdi);
    ret = RenderSetVoiceVolumeImpl(renderIns, volumeHdi);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("RenderSetVolumeImpl fail!");
        return HDF_FAILURE;
    }
    
    return HDF_SUCCESS;
}

static bool RenderGetMuteImpl(struct AlsaRender *renderIns)
{
    return renderIns->muteState;
}

static int32_t RenderSetMuteImpl(struct AlsaRender *renderIns, bool muteFlag)
{
    AUDIO_FUNC_LOGE("RenderSetMuteImpl enter muteFlag: %{public}d!", muteFlag);
    int32_t ret;

    RenderData *priData = RenderGetPriData(renderIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(priData);

    struct AlsaMixerCtlElement elem;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;

    SndElementItemInit(&elem);
    elem.numid = CAPTURE_MUTE_NUMID;
    elem.name = CAPTURE_MUTE_PATH;
    elem.value = muteFlag ? OPEN_MIC : CLOSE_MIC;

    ret = SndElementWrite(cardIns, &elem);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("write capture fail!");
        return HDF_FAILURE;
    }

    renderIns->muteState = muteFlag;
    
    return HDF_SUCCESS;
}

static int32_t UpdateDeviceSo(struct AlsaRender *renderIns)
{
    int32_t ret;
    if (!g_dlHandle) {
        AUDIO_FUNC_LOGE("open libsprd_mock_effect_lib.z.so failed!");
        return HDF_FAILURE;
    }
    UpdateFunc updateDevice = (UpdateFunc)dlsym(g_dlHandle, "update_device");
    if (updateDevice == NULL) {
        AUDIO_FUNC_LOGE("update_device not defined or exported!");
        return HDF_FAILURE;
    }
    ret = updateDevice(renderIns->descPins);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("render update_device faild");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t RenderStartImpl(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData)
{
    AUDIO_FUNC_LOGI("RenderStartImpl enter g_currentScene: %{public}d!", g_currentScene);
    int32_t ret;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);

    if (g_currentScene == AUDIO_IN_CALL) {
        if (cardIns->pcmHandle && CaptureHandle) {
            ret = snd_pcm_start(cardIns->pcmHandle);
            AUDIO_FUNC_LOGI("RenderStartImpl use snd_pcm_start ret:%{public}d", ret);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("snd_pcm_start fail: %{public}s, ret:%{public}d", snd_strerror(ret), ret);
                return HDF_FAILURE;
            }

            ret = snd_pcm_start(CaptureHandle);
            AUDIO_FUNC_LOGI("CaptureStartImpl use snd_pcm_start ret:%{public}d", ret);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("snd_pcm_start fail: %{public}s, ret: %{public}d", snd_strerror(ret), ret);
                return HDF_FAILURE;
            }

            ret = UpdateAudioRenderRoute(renderIns, handleData);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams UpdateAudioRenderRoute fail");
                return HDF_FAILURE;
            }

            ret = UpdateDeviceSo(renderIns);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("render UpdateDevice faild");
                return HDF_FAILURE;
            }
            return HDF_SUCCESS;
        }
        AUDIO_FUNC_LOGI("RenderStartImpl  call  cardIns->devName: %{public}s!", cardIns->devName);
        return HDF_FAILURE;
    } else {
        if (!SndisBusy(cardIns)) {
            AUDIO_FUNC_LOGI("RenderStartImpl pcmHandle is NULL");
            ret = ReOpenPcmAndSetParams(renderIns, handleData);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("ReOpenPcmAndSetParams fail");
                return HDF_FAILURE;
            }
        }
    }

    return HDF_SUCCESS;
}

static int32_t RenderStopImpl(struct AlsaRender *renderIns)
{
    AUDIO_FUNC_LOGI("RenderStopImpl enter!");

    if (g_currentScene == AUDIO_IN_CALL) {
        return HDF_SUCCESS;
    }
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns->soundCard.pcmHandle);
    
    snd_pcm_drain(renderIns->soundCard.pcmHandle);
        
    PcmCloseHandle(&renderIns->soundCard);

    AUDIO_FUNC_LOGI("RenderStopImpl end!");
    return HDF_SUCCESS;
}

static int32_t RenderGetGainThresholdImpl(struct AlsaRender *renderIns, float *gainMin, float *gainMax)
{
    AUDIO_FUNC_LOGI("Not support gain operation");
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    (void)gainMin;
    (void)gainMax;
    return HDF_SUCCESS;
}

static int32_t RenderGetGainImpl(struct AlsaRender *renderIns, float *volume)
{
    AUDIO_FUNC_LOGI("Not support gain operation");
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    (void)volume;
    return HDF_SUCCESS;
}

static int32_t RenderSetGainImpl(struct AlsaRender *renderIns, float volume)
{
    AUDIO_FUNC_LOGI("Not support gain operation");
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    (void)volume;
    return HDF_SUCCESS;
}

static int32_t RenderGetChannelModeImpl(struct AlsaRender *renderIns, enum AudioChannelMode *mode)
{
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    (void)mode;
    return HDF_SUCCESS;
}

static int32_t RenderSetChannelModeImpl(struct AlsaRender *renderIns, enum AudioChannelMode mode)
{
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    (void)mode;
    return HDF_SUCCESS;
}

 static int32_t SaveHwParams(struct AlsaSoundCard *cardIns, const struct AudioHwRenderParam *handleData)
{
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(handleData);

    cardIns->hwParams.streamType = AUDIO_RENDER_STREAM;
    cardIns->hwParams.channels = handleData->frameRenderMode.attrs.channelCount;
    cardIns->hwParams.rate = handleData->frameRenderMode.attrs.sampleRate;
    cardIns->hwParams.periodSize = handleData->frameRenderMode.periodSize;
    cardIns->hwParams.periodCount = handleData->frameRenderMode.periodCount;
    cardIns->hwParams.format = handleData->frameRenderMode.attrs.format;
    cardIns->hwParams.period = handleData->frameRenderMode.attrs.period;
    cardIns->hwParams.frameSize = handleData->frameRenderMode.attrs.frameSize;
    cardIns->hwParams.isBigEndian = handleData->frameRenderMode.attrs.isBigEndian;
    cardIns->hwParams.isSignedData = handleData->frameRenderMode.attrs.isSignedData;
    cardIns->hwParams.startThreshold = handleData->frameRenderMode.attrs.startThreshold;
    cardIns->hwParams.stopThreshold = handleData->frameRenderMode.attrs.stopThreshold;
    cardIns->hwParams.silenceThreshold = handleData->frameRenderMode.attrs.silenceThreshold;
#ifdef SUPPORT_ALSA_CHMAP
    /* param 2 by handleData->frameRenderMode.attrs.channelsName, sample channelsName is "FL, FR" */
    if (GetChannelsNameFromUser(cardIns, "FL, FR") != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("GetChannelsNameFromUser failed");
        return HDF_FAILURE;
    }
#endif

    return HDF_SUCCESS;
}

static int32_t ConfigureCaptur(snd_pcm_t *handle, snd_pcm_hw_params_t *hwParams)
{
    snd_pcm_uframes_t size = 0;
    int32_t ret;
    size = CAPTURE_PERIOD_SIZE_CALL;
    ret = snd_pcm_hw_params_set_period_size_near(handle, hwParams, &size, 0);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SetHWParamsCall Set period size failed!");
        return ret;
    }

    size = CAPTURE_BUFFER_SIZE_CALL;
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

static int32_t SetHWParamsCapture(snd_pcm_t *pcmHandle)
{
    AUDIO_FUNC_LOGI("SetHWParamsCapture begin.");
    int32_t ret;
    int32_t dir = 0;
    uint32_t channel = CHANNEL_CALL;
    uint32_t rRate = RATE_CALL;
    snd_pcm_format_t pcmFormat = SND_PCM_FORMAT_S16_LE;
    snd_pcm_hw_params_t *hwParams = NULL;
    snd_pcm_t *handle = pcmHandle;
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
    }

    ret = ConfigureCaptur(handle, hwParams);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("ConfigureCaptur Unable to set hw params for capture: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

#ifdef SUPPORT_ALSA_CHMAP
static void PrintChannels(const snd_pcm_chmap_t *map)
{
    char tmp[CHMAP_NAME_LENGHT_MAX] = {0};
    if (snd_pcm_chmap_print(map, sizeof(tmp), tmp) > 0) {
        HDF_LOGI("print_channels: %{public}s.", tmp);
    }
}

static int32_t QueryChmaps(snd_pcm_t *pcm)
{
    snd_pcm_chmap_query_t **pChmap = NULL;
    snd_pcm_chmap_query_t *chmap = NULL;
    const char *champType = NULL;
    snd_pcm_chmap_query_t **hwChmap = snd_pcm_query_chmaps(pcm);
    if (hwChmap == NULL) {
        AUDIO_FUNC_LOGE("This sound card has no chmap component, cannot query maps.");
        return HDF_FAILURE;
    }

    for (pChmap = hwChmap; (chmap = *pChmap) != NULL; pChmap++) {
        champType = snd_pcm_chmap_type_name(chmap->type);
        HDF_LOGI("Channel Type = %{public}s, Channels = %{public}d.", champType, chmap->map.channels);
        if (strncmp(champType, CHANNEL_MAP_TYPE_FIXED, strlen(CHANNEL_MAP_TYPE_FIXED)) == 0) {
            HDF_LOGW("Fixed channel type does not support modification temporarily!");
        }
        PrintChannels(&chmap->map);
    }

    snd_pcm_free_chmaps(hwChmap);
    return HDF_SUCCESS;
}

static int32_t SetChmap(snd_pcm_t *pcm, struct AudioPcmHwParams *hwRenderParams)
{
    if (hwRenderParams == NULL || hwRenderParams->channelsName == NULL) {
        AUDIO_FUNC_LOGE("Parameter is NULL!");
        return HDF_FAILURE;
    }

    snd_pcm_chmap_t *chmap = snd_pcm_chmap_parse_string(hwRenderParams->channelsName);
    if (chmap == NULL) {
        AUDIO_FUNC_LOGE("parse chmap error!");
        return HDF_FAILURE;
    }

    if (snd_pcm_set_chmap(pcm, chmap) < 0) {
        AUDIO_FUNC_LOGE("Cannot set chmap!");
        free((void *)chmap);
        return HDF_ERR_NOT_SUPPORT;
    }
    free((void *)chmap);

    chmap = snd_pcm_get_chmap(pcm);
    if (chmap == NULL) {
        AUDIO_FUNC_LOGE("Cannot get chmap!");
        return HDF_ERR_NOT_SUPPORT;
    }

    PrintChannels(chmap);
    free((void *)chmap);
    return HDF_SUCCESS;
}

static int32_t RenderHwParamsChmaps(struct AlsaSoundCard *cardIns)
{
    if (QueryChmaps(cardIns->pcmHandle) != HDF_SUCCESS) {
        AUDIO_FUNC_LOGW("QueryChmaps failed.");
        return HDF_SUCCESS;
    }
    if (SetChmap(cardIns->pcmHandle, &cardIns->hwParams) != HDF_SUCCESS) {
        AUDIO_FUNC_LOGW("SetChmap failed.");
    }

    return HDF_SUCCESS;
}
#endif

static int32_t EnableAlsa(snd_pcm_t *handle, snd_pcm_hw_params_t *params)
{
    int32_t ret;

    /* set hardware resampling,enable alsa-lib resampling */
    ret = snd_pcm_hw_params_set_rate_resample(handle, params, 1);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Resampling setup failed for playback: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }
    /* set the interleaved read/write format */
    snd_pcm_access_mask_t *mask = malloc(snd_pcm_access_mask_sizeof());
    snd_pcm_access_mask_none(mask);
    snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_INTERLEAVED);
    snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_NONINTERLEAVED);
    snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_COMPLEX);
    ret = snd_pcm_hw_params_set_access_mask(handle, params, mask);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Access type not available for playback: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t SetHWParamsSubVdi(
    snd_pcm_t *handle, snd_pcm_hw_params_t *params, const struct AudioPcmHwParams *hwParams)
{
    int32_t ret;
    snd_pcm_format_t pcmFormat = SND_PCM_FORMAT_S16_LE;
    CHECK_NULL_PTR_RETURN_DEFAULT(handle);
    CHECK_NULL_PTR_RETURN_DEFAULT(params);

    ret = EnableAlsa(handle, params);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("SndConverAlsaPcmFormat error.");
        return HDF_FAILURE;
    }

    ret = SndConverAlsaPcmFormat(hwParams, &pcmFormat);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("SndConverAlsaPcmFormat error.");
        return HDF_FAILURE;
    }
    /* set the sample format */
    AUDIO_FUNC_LOGI("SetHWParamsSubVdi pcmFormat: %{public}d", pcmFormat);
    if (g_currentScene != AUDIO_IN_CALL) {
        ret = snd_pcm_hw_params_set_format(handle, params, pcmFormat);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("Sample format not available for playback: %{public}s, format: %{public}d",
                snd_strerror(ret), pcmFormat);
            return HDF_FAILURE;
        }
        /* set the count of channels */
        AUDIO_FUNC_LOGI("SetHWParamsSubVdi hwParams->channels: %{public}d", hwParams->channels);
        ret = snd_pcm_hw_params_set_channels(handle, params, hwParams->channels);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("Channels count (%{public}u) not available for playbacks: %{public}s", hwParams->channels,
                snd_strerror(ret));
            return HDF_FAILURE;
        }
    } else {
        /* set the sample format */
        pcmFormat = SND_PCM_FORMAT_S16_LE;
        ret = snd_pcm_hw_params_set_format(handle, params, pcmFormat);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("Sample format not available for playback: %{public}s", snd_strerror(ret));
            return HDF_FAILURE;
        }
        /* set the count of channels */
        uint32_t channel = CHANNEL_CALL;
        ret = snd_pcm_hw_params_set_channels(handle, params, channel);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("Channels count (%{public}u) not available for playbacks: %{public}s", channel,
                snd_strerror(ret));
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

static int32_t SetHWRateVdi(snd_pcm_t *handle, snd_pcm_hw_params_t *params, uint32_t *rate)
{
    int32_t ret;
    uint32_t rRate;
    int dir = 0; /* dir Value range (-1,0,1) */
    CHECK_NULL_PTR_RETURN_DEFAULT(handle);
    CHECK_NULL_PTR_RETURN_DEFAULT(params);
    CHECK_NULL_PTR_RETURN_DEFAULT(rate);

    /* set the stream rate */
    if (g_currentScene != AUDIO_IN_CALL) {
        rRate = *rate;
        ret = snd_pcm_hw_params_set_rate_near(handle, params, &rRate, &dir);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("Rate %{public}uHz not available for playback: %{public}s.", *rate, snd_strerror(ret));
            return HDF_FAILURE;
        }

        if (rRate != *rate) {
            ret = snd_pcm_hw_params_set_rate_near(handle, params, &rRate, &dir);
            if (ret < 0) {
                AUDIO_FUNC_LOGE("Rate %{public}uHz not available for playback: %{public}s.", *rate, snd_strerror(ret));
                return HDF_FAILURE;
            }
        }
    } else {
        rRate = RATE_CALL;
        ret = snd_pcm_hw_params_set_rate_near(handle, params, &rRate, &dir);
    }
    AUDIO_FUNC_LOGI("SetHWRateVdi rRate: %{public}d", rRate);
    /* Update to hardware supported rate */
    *rate = rRate;

    return HDF_SUCCESS;
}

static int32_t SetHWRateParamsVdi(struct AlsaSoundCard *cardIns, snd_pcm_hw_params_t *hwParams)
{
    if (snd_pcm_hw_params_any(cardIns->pcmHandle, hwParams) < 0) {
        AUDIO_FUNC_LOGE("No configurations available");
        return HDF_FAILURE;
    }
    AUDIO_FUNC_LOGI("SetHWParamsVdi format:%{public}d", cardIns->hwParams.format);
    if (SetHWParamsSubVdi(cardIns->pcmHandle, hwParams, &cardIns->hwParams) != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SetHWParamsSubVdi failed!");
        return HDF_FAILURE;
    }
    if (SetHWRateVdi(cardIns->pcmHandle, hwParams, &(cardIns->hwParams.rate)) != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SetHWRateVdi failed!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t SetHWParamsVdi(struct AlsaSoundCard *cardIns)
{
    snd_pcm_uframes_t size;
    snd_pcm_hw_params_t *hwParams = NULL;
    struct AlsaRender *renderIns = (struct AlsaRender*)cardIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns->pcmHandle);

    snd_pcm_hw_params_alloca(&hwParams);
    if (SetHWRateParamsVdi(cardIns, hwParams) != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SetHWRateParamsVdi failed!");
        return HDF_FAILURE;
    }

    AUDIO_FUNC_LOGI("bufferTime:%{public}ud, periodTime:%{public}ud", renderIns->bufferTime, renderIns->periodTime);
    if (g_currentScene != AUDIO_IN_CALL) {
        /* default time bufferTime:500000d, periodTime:125000d */
        /* default size bufferSize:24000, periodSize:6000 */
        renderIns->periodSize = PERIOD_SIZE_DEFAULT;
        renderIns->bufferSize = BUFFER_SIZE_DEFAULT;
        size = renderIns->periodSize;
        if (snd_pcm_hw_params_set_period_size_near(cardIns->pcmHandle, hwParams, &size, 0) < 0) {
            AUDIO_FUNC_LOGE("call snd_pcm_hw_params_set_period_size_near failed");
            return HDF_FAILURE;
        }

        size = renderIns->bufferSize;
        if (snd_pcm_hw_params_set_buffer_size_near(cardIns->pcmHandle, hwParams, &size) < 0) {
            AUDIO_FUNC_LOGE("call snd_pcm_hw_params_set_buffer_size_near failed");
            return HDF_FAILURE;
        }
    } else {
        renderIns->periodSize = PERIOD_SIZE_CALL;
        renderIns->bufferSize = BUFFER_SIZE_CALL;

        size = renderIns->periodSize;
        if (snd_pcm_hw_params_set_period_size_near(cardIns->pcmHandle, hwParams, &size, 0) < 0) {
            AUDIO_FUNC_LOGE("call snd_pcm_hw_params_set_period_size_near failed");
            return HDF_FAILURE;
        }

        size = renderIns->bufferSize;
        if (snd_pcm_hw_params_set_buffer_size_near(cardIns->pcmHandle, hwParams, &size) < 0) {
            AUDIO_FUNC_LOGE("call snd_pcm_hw_params_set_buffer_size_near failed");
            return HDF_FAILURE;
        }
    }
     
    AUDIO_FUNC_LOGI("bufferSize:%{public}ld, periodSize:%{public}ld", renderIns->bufferSize, renderIns->periodSize);

    if (snd_pcm_hw_params(cardIns->pcmHandle, hwParams) < 0) {
        AUDIO_FUNC_LOGE("Unable to set hw params for playback");
        return HDF_FAILURE;
    }

    cardIns->canPause = false;
    return HDF_SUCCESS;
}

static int32_t WriteToPlayback(snd_pcm_t *handle, snd_pcm_sw_params_t *swParams, struct AlsaRender *renderIns)
{
    int32_t ret;
    int32_t val = 1; /* val 0 = disable period event, 1 = enable period event */
    snd_pcm_sframes_t stopThresholdSize = -1;
    ret = snd_pcm_sw_params_set_stop_threshold(handle, swParams, stopThresholdSize);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Unable to set stop threshold mode for playback: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    /* enable period events when requested */
    if (renderIns->periodEvent) {
        ret = snd_pcm_sw_params_set_period_event(handle, swParams, val);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("Unable to set period event: %{public}s", snd_strerror(ret));
            return HDF_FAILURE;
        }
    }

    /* write the parameters to the playback device */
    ret = snd_pcm_sw_params(handle, swParams);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Unable to set sw params for playback: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t SetSWParamsVdi(struct AlsaSoundCard *cardIns)
{
    int32_t ret;
    snd_pcm_sw_params_t *swParams = NULL;
    snd_pcm_t *handle = cardIns->pcmHandle;
    struct AlsaRender *renderIns = (struct AlsaRender *)cardIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(handle);

    snd_pcm_sw_params_alloca(&swParams);

    /* get the current swparams */
    ret = snd_pcm_sw_params_current(handle, swParams);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Unable to determine current swparams for playback: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }
    /* start the transfer when the buffer is almost full: */
    /* (buffer_size / avail_min) * avail_min */
    if (renderIns->periodSize == 0) {
        AUDIO_FUNC_LOGE("g_periodSize=0");
        return HDF_FAILURE;
    }

    if (g_currentScene == AUDIO_IN_CALL) {
        ret = snd_pcm_sw_params_set_avail_min(handle, swParams, 0);
        ret = snd_pcm_sw_params_set_start_threshold(handle, swParams, 0);
        ret = snd_pcm_sw_params_set_silence_size(handle, swParams, 0);
        ret = snd_pcm_sw_params_set_silence_threshold(handle, swParams, 0);
    } else {
        ret = snd_pcm_sw_params_set_avail_min(handle, swParams, renderIns->periodEvent ? renderIns->bufferSize :
            renderIns->periodSize);
        AUDIO_FUNC_LOGI("SetSWParams use snd_pcm_sw_params_set_avail_min");
        if (ret < 0) {
            AUDIO_FUNC_LOGE("Unable to set avail min for playback: %{public}s", snd_strerror(ret));
            return HDF_FAILURE;
        }
        ret = snd_pcm_sw_params_set_start_threshold(handle, swParams, (renderIns->bufferSize / renderIns->periodSize)
            * renderIns->periodSize);
    }

    if (ret < 0) {
        AUDIO_FUNC_LOGE("Unable to set start threshold mode for playback: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    ret = WriteToPlayback(handle, swParams, renderIns);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Unable to write the parameters to the playback device: %{public}s", snd_strerror(ret));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RenderSetParamsVdi(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData)
{
    int32_t ret;
    int bitsPerSample;
    snd_pcm_format_t fmt;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);

    SaveHwParams(&renderIns->soundCard, handleData);
    ret = SetHWParamsVdi(cardIns);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Setting of hwparams failed.");
        return HDF_FAILURE;
    }

    ret = SetSWParamsVdi(cardIns);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Setting of swparams failed.");
        return HDF_FAILURE;
    }

#ifdef SUPPORT_ALSA_CHMAP
    ret = RenderHwParamsChmaps(&renderIns->soundCard);
    if (ret < 0) {
        AUDIO_FUNC_LOGE("Setting of chmaps failed.");
    }
#endif

    SndConverAlsaPcmFormat(&cardIns->hwParams, &fmt);
    bitsPerSample = snd_pcm_format_physical_width(fmt);
    cardIns->hwParams.bitsPerFrame = bitsPerSample * cardIns->hwParams.channels;

    return HDF_SUCCESS;
}

static int32_t RenderSetHwParamsImpl(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData)
{
    int32_t ret;
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(handleData);
    ret = RenderSetParamsVdi(renderIns, handleData);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("Render set parameters failed!");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t RenderSetTurningImpl()
{
    AUDIO_FUNC_LOGI("RenderSetTurningImpl enter");
    int32_t ret;
    g_dlHandle = dlopen(SPRD_EFFECT_SO_PATH, RTLD_LAZY);
    if (!g_dlHandle) {
        AUDIO_FUNC_LOGE("open libsprd_mock_effect_lib.z.so failed!");
        return HDF_FAILURE;
    }
    UploadFunc uploadAudio = (UploadFunc)dlsym(g_dlHandle, "upload_audio_profile_param_from_turnings");
    if (uploadAudio == NULL) {
        AUDIO_FUNC_LOGE("upload_audio_profile_param_from_turnings not defined or exported!");
        return HDF_FAILURE;
    }
    ret = uploadAudio();
    if (ret != AUDIO_SUCCESS) {
        AUDIO_FUNC_LOGE("load profile param from turnings failed!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RenderReadFromVoiceImpl(struct AlsaRender *renderIns, const char *adapterName)
{
    AUDIO_FUNC_LOGI("RenderReadFromVoiceImpl enter");
    (void)renderIns;
    int32_t ret;
    if (adapterName == NULL || strlen(adapterName) == 0) {
        AUDIO_FUNC_LOGE("Invalid adapterName!");
        return HDF_FAILURE;
    }

    if (!g_dlHandle) {
        AUDIO_FUNC_LOGE("open libsprd_mock_effect_lib.z.so failed!");
        return HDF_FAILURE;
    }
    InitFunc deviceInit = (InitFunc)dlsym(g_dlHandle, "device_init");
    if (deviceInit == NULL) {
        AUDIO_FUNC_LOGE("device_init not defined or exported!");
        return HDF_FAILURE;
    }

    ret = deviceInit(adapterName);
    if (ret != AUDIO_SUCCESS) {
        AUDIO_FUNC_LOGE("read voice param from dev/audio_pipe_voice failed!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t RenderCloseVoiceImpl(struct AlsaRender *renderIns)
{
    AUDIO_FUNC_LOGE("RenderCloseVoiceImpl enter");
    (void)renderIns;
    if (!g_dlHandle) {
        AUDIO_FUNC_LOGE("open libsprd_mock_effect_lib.z.so failed!");
        return HDF_FAILURE;
    }
    CloseFunc deviceClose = (CloseFunc)dlsym(g_dlHandle, "device_close");
    if (deviceClose == NULL) {
        AUDIO_FUNC_LOGE("device_close not defined or exported!");
        return HDF_FAILURE;
    }
    AUDIO_FUNC_LOGE("RenderCloseVoiceImpl end");
    return HDF_SUCCESS;
}

static int32_t RenderSetVoiceVolumeImpl(struct AlsaRender *renderIns, float volume)
{
    AUDIO_FUNC_LOGE("RenderSetVoiceVolumeImpl enter");
    int32_t ret;
    struct AlsaMixerCtlElement elem;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);
    if (volume < 0) {
        volume = 0;
    } else if (volume > 1) {
        volume = 1;
    }
    AUDIO_FUNC_LOGI("RenderSetVoiceVolumeImpl Set voice volume:%{public}f", volume);
    uint32_t hdiVolume = volume * AUDIO_VOLUME_TO_HDI_DIVISOR;
    AUDIO_FUNC_LOGI("RenderSetVoiceVolumeImpl Set voice hdiVolume:%{public}d", hdiVolume);

    SndElementItemInit(&elem);
    elem.numid = VOICE_VOLUME_ID;
    elem.name = VOICE_VOLUME_PATH;
    ret = SndElementWriteInt(cardIns, &elem, hdiVolume);
    if (ret < HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("SndElementWriteInt failed!");
        return HDF_FAILURE;
    };

    return HDF_SUCCESS;
}

static int32_t RenderWriteiVdi(snd_pcm_t *pcm, const struct AudioHwRenderParam *handleData,
    const struct AudioPcmHwParams *hwParams)
{
    CHECK_NULL_PTR_RETURN_DEFAULT(pcm);
    int32_t ret;
    int32_t offset;
    long frames;
    char *dataBuf;
    size_t sbufFrameSize;
    snd_pcm_state_t state;
    int32_t tryNum = AUDIO_ALSALIB_RETYR;
    if (g_currentScene == AUDIO_IN_CALL) {
        AUDIO_FUNC_LOGI("RenderWriteiVdi scene is call, no need write");
        return HDF_FAILURE;
    }
    state = snd_pcm_state(pcm);
    if (state == SND_PCM_STATE_SETUP) {
        ret = snd_pcm_prepare(pcm);
        if (ret < 0) {
            AUDIO_FUNC_LOGE("snd_pcm_prepare fail: %{public}s", snd_strerror(ret));
            return HDF_FAILURE;
        }
    }

    sbufFrameSize = (size_t)handleData->frameRenderMode.bufferFrameSize;
    dataBuf = handleData->frameRenderMode.buffer;
    offset = hwParams->bitsPerFrame / BIT_COUNT_OF_BYTE;
    while (sbufFrameSize > 0) {
        frames = snd_pcm_mmap_writei(pcm, dataBuf, sbufFrameSize);
        AUDIO_FUNC_LOGI("snd_pcm_mmap_writei end frames: %{public}ld", frames);
        if (frames > 0) {
            sbufFrameSize -= frames;
            dataBuf += frames * offset;
        } else if (frames == -EAGAIN) {
            snd_pcm_wait(pcm, PCM_WAIT_TIMEOUT_MS);
            tryNum--;
            if (tryNum == 0) {
                return HDF_SUCCESS;
            }
        } else if (frames == -EBADFD) {
            AUDIO_FUNC_LOGE("render PCM is not in the right state: %{public}s", snd_strerror(frames));
            snd_pcm_prepare(pcm);
            return HDF_FAILURE;
        } else {
            /* -ESTRPIPE: a suspend event occurred,
             * stream is suspended and waiting for an application recovery.
             * -EPIPE: an underrun occurred.
             */
            ret = snd_pcm_recover(pcm, frames, 0); // 0 for open render recover log.
            if (ret < 0) {
                AUDIO_FUNC_LOGE("snd_pcm_writei failed: %{public}s", snd_strerror(ret));
                return HDF_FAILURE;
            }
        }
    }

    return HDF_SUCCESS;
}

int32_t RenderWriteVdiImpl(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData)
{
    int32_t ret;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard*)renderIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns->pcmHandle);

    if (cardIns->pauseState) {
        AUDIO_FUNC_LOGE("Currently in pause, please check!");
        return HDF_FAILURE;
    }

    ret = RenderWriteiVdi(cardIns->pcmHandle, handleData, &cardIns->hwParams);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("RenderWriteiVdi failed!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t RenderUpdateRouterImpl(struct AlsaRender *renderIns, const struct AudioHwRenderParam *handleData)
{
    int32_t ret;
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard*)renderIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(renderIns);
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns->pcmHandle);

    ret = UpdateAudioRenderRoute(renderIns, handleData);
    if (ret != HDF_SUCCESS) {
        AUDIO_FUNC_LOGE("RenderUpdateRouterImpl failed!");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t RenderOverrideFunc(struct AlsaRender *renderIns)
{
    struct AlsaSoundCard *cardIns = (struct AlsaSoundCard *)renderIns;
    CHECK_NULL_PTR_RETURN_DEFAULT(cardIns);

    if (cardIns->cardType == SND_CARD_PRIMARY) {
        renderIns->Init = RenderInitImpl;
        renderIns->SelectScene = RenderSelectSceneImpl;
        renderIns->Start = RenderStartImpl;
        renderIns->Stop = RenderStopImpl;
        renderIns->GetVolThreshold = RenderGetVolThresholdImpl;
        renderIns->GetVolume = RenderGetVolumeImpl;
        renderIns->SetVolume = RenderSetVolumeImpl;
        renderIns->GetGainThreshold = RenderGetGainThresholdImpl;
        renderIns->GetGain = RenderGetGainImpl;
        renderIns->SetGain = RenderSetGainImpl;
        renderIns->GetMute = RenderGetMuteImpl;
        renderIns->SetMute = RenderSetMuteImpl;
        renderIns->GetChannelMode = RenderGetChannelModeImpl;
        renderIns->SetChannelMode = RenderSetChannelModeImpl;
        renderIns->UpdateRouter = RenderUpdateRouterImpl;

        renderIns->SetParams = RenderSetHwParamsImpl;
        renderIns->SetTurning = RenderSetTurningImpl;
        renderIns->ReadFromVoice = RenderReadFromVoiceImpl;
        renderIns->CloseVoice = RenderCloseVoiceImpl;
        renderIns->SetVoiceVolume = RenderSetVoiceVolumeImpl;
        renderIns->Write = RenderWriteVdiImpl;
    }

    return HDF_SUCCESS;
}

int32_t RenderGetSceneDev(enum AudioCategory scene)
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
