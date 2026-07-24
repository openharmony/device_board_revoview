/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "v4l2_dev.h"

#include <dlfcn.h>
#include <sys/prctl.h>

#include "securec.h"
#define RECTREGION     200
#define LEN            4

namespace OHOS::Camera {
std::map<std::string, std::string> HosV4L2Dev::deviceMatch =
    HosV4L2Dev::CreateDevMap();
std::map<std::string, int> HosV4L2Dev::fdMatch = HosV4L2Dev::CreateFdMap();
std::mutex HosV4L2Dev::deviceFdLock_ = {};
V4l2AdapterInterface* HosV4L2Dev::v4l2Handle_ =
    HosV4L2Dev::CreateV4l2AdapterInterface();

static constexpr uint32_t WATING_TIME = 1000 * 100;

V4l2AdapterInterface* HosV4L2Dev::CreateV4l2AdapterInterface()
{
    void* handler = dlopen("/vendor/lib64/libcamv4l2adapter.z.so", RTLD_LAZY);
    if (handler == nullptr) {
        CAMERA_LOGE(
            "failed to dlopen  /vendor/lib64/libcamv4l2adapter.z.so, "
            "%{public}s",
            dlerror());
        return nullptr;
    }

    typedef V4l2AdapterInterface* CreateInstanceFunc();
    CreateInstanceFunc* createInstance =
        (CreateInstanceFunc*)dlsym(handler, "CreateV4l2AdapterInstance");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        CAMERA_LOGE("Cannot load symbol CreateV4l2AdapterInstance, %{public}s",
                    dlerror());
        return nullptr;
    }

    V4l2AdapterInterface* v4l2Handle = createInstance();
    if (v4l2Handle == nullptr) {
        CAMERA_LOGE("CreateV4l2AdapterInstance, failed");
    }

    return v4l2Handle;
}

HosV4L2Dev::HosV4L2Dev() { currStartQueueBuffNum = 0; }

HosV4L2Dev::~HosV4L2Dev() { currStartQueueBuffNum = 0; }

int HosV4L2Dev::GetCurrentFd(const std::string& cameraID)
{
    auto itr = HosV4L2Dev::fdMatch.find(cameraID);
    if (itr == HosV4L2Dev::fdMatch.end()) {
        CAMERA_LOGE("error: GetCurrentFd no camera fd\n");
        return RCERRORFD;
    }

    return itr->second;
}

RetCode HosV4L2Dev::start(const std::string& cameraID)
{
    std::string devName;
    int fd = 0;

    CAMERA_LOGD("HosV4L2Dev::start enter %{public}s\n", cameraID.c_str());

    if (myFileFormat_ == nullptr) {
        myFileFormat_ = std::make_shared<HosFileFormat>();
        if (myFileFormat_ == nullptr) {
            CAMERA_LOGE(
                "error: InitMatch: myFileFormat_ make_shared is NULL\n");
            return RC_ERROR;
        }
    }

    auto itr = HosV4L2Dev::deviceMatch.find(cameraID);
    if (itr == HosV4L2Dev::deviceMatch.end()) {
        CAMERA_LOGE("error:find V4L2 devname fail\n");
        return RC_ERROR;
    }
    devName = itr->second;

    fd = myFileFormat_->V4L2OpenDevice(devName);
    if (fd < 0) {
        CAMERA_LOGE(
            "error:myFileFormat_->V4L2OpenDevice fail fd == %{public}d\n", fd);
        return RC_ERROR;
    }

    bufferType_ =
        static_cast<enum v4l2_buf_type>(myFileFormat_->V4L2SearchBufType(fd));
    if (bufferType_ == V4L2_BUF_TYPE_PRIVATE) {
        CAMERA_LOGE(
            "error:myFileFormat_->V4L2SearchBufType bufferType_ == 0\n");
        return RC_ERROR;
    }

    std::lock_guard<std::mutex> l(HosV4L2Dev::deviceFdLock_);
    HosV4L2Dev::fdMatch.insert(std::make_pair(cameraID, fd));

    return RC_OK;
}

RetCode HosV4L2Dev::stop(const std::string& cameraID)
{
    int fd = 0;

    CAMERA_LOGD("HosV4L2Dev::stop enter %{public}s\n", cameraID.c_str());

    if (myFileFormat_ == nullptr) {
        CAMERA_LOGE("HosV4L2Dev::stop myFileFormat_ == nullptr\n");
        return RC_ERROR;
    }

    auto itr = HosV4L2Dev::fdMatch.find(cameraID);
    if (itr == HosV4L2Dev::fdMatch.end()) {
        CAMERA_LOGE("HosV4L2Dev::stop GetCurrentFd error\n");
        return RC_ERROR;
    }

    fd = itr->second;
    if (fd < 0) {
        CAMERA_LOGE("HosV4L2Dev::stop fd error = %{public}d\n", fd);
        return RC_ERROR;
    }

    myFileFormat_->V4L2CloseDevice(fd);

    std::lock_guard<std::mutex> l(HosV4L2Dev::deviceFdLock_);
    HosV4L2Dev::fdMatch.erase(itr);
    currStartQueueBuffNum = 0;
    return RC_OK;
}

RetCode HosV4L2Dev::Init(std::vector<std::string>& cameraIDs)
{
    auto myFileFormat = std::make_shared<HosFileFormat>();
    if (myFileFormat == nullptr) {
        CAMERA_LOGE("error: InitMatch: myFileFormat_ make_shared is NULL\n");
        return RC_ERROR;
    }

    myFileFormat->V4L2MatchDevice(cameraIDs);

    return RC_OK;
}

void HosV4L2Dev::Release()
{
    if (v4l2Handle_ != nullptr) {
        delete v4l2Handle_;
        v4l2Handle_ = nullptr;
    }
}

RetCode HosV4L2Dev::ReqBuffers(const std::string& cameraID,
                               unsigned int buffCont)
{
    int rc = 0;
    int fd = 0;

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("error: ReqBuffers: GetCurrentFd error\n");
        return RC_ERROR;
    }

    if (myBuffers_ == nullptr) {
        myBuffers_ = std::make_shared<HosV4L2Buffers>(memoryType_, bufferType_);
        if (myBuffers_ == nullptr) {
            CAMERA_LOGE("error: ReqBuffers: myBuffers_ make_shared is NULL\n");
            return RC_ERROR;
        }
    }

    if (buffCont > 0) {
        myBuffers_->ReSetBufCont();
    }

    rc = myBuffers_->V4L2ReqBuffers(fd, buffCont);
    if (rc == RC_ERROR) {
        CAMERA_LOGE("error: ReqBuffers: V4L2ReqBuffers error\n");
        return RC_ERROR;
    }

    return RC_OK;
}

RetCode HosV4L2Dev::CreatBuffer(const std::string& cameraID,
                                const std::shared_ptr<FrameSpec>& frameSpec)
{
    int rc = 0;
    int fd = 0;

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("error: ReqBuffers: GetCurrentFd error\n");
        return RC_ERROR;
    }

    if (frameSpec == nullptr || myBuffers_ == nullptr) {
        CAMERA_LOGE("error: Creatbuffer frameSpec or myBuffers_ is NULL\n");
        return RC_ERROR;
    }

    CAMERA_LOGD("Creatbuffer frameSpec->buffer index == %{public}d\n",
                frameSpec->buffer_->GetIndex());

    rc = myBuffers_->V4L2AllocBuffer(fd, frameSpec);
    if (rc == RC_ERROR) {
        CAMERA_LOGE("error: Creatbuffer: V4L2AllocBuffer error\n");
        return RC_ERROR;
    }

    return RC_OK;
}

RetCode HosV4L2Dev::CreatBuffer(const std::string& cameraID, int buffcont)
{
    int rc = 0;
    int fd = 0;

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("error: CreatBuffer: GetCurrentFd error\n");
        return RC_ERROR;
    }

    if (myBuffers_ == nullptr) {
        CAMERA_LOGE("error: Creatbuffer frameSpec or myBuffers_ is NULL\n");
        return RC_ERROR;
    }

    CAMERA_LOGD("Creatbuffer cameraID == %{public}s,buffcont= %{public}d\n",
                cameraID.c_str(), buffcont);

    rc = myBuffers_->V4L2AllocBuffer(fd, buffcont);
    if (rc == RC_ERROR) {
        CAMERA_LOGE("error: Creatbuffer: V4L2AllocBuffer error\n");
        return RC_ERROR;
    }

    return RC_OK;
}

RetCode HosV4L2Dev::DestoryBuffers() { return RC_OK; }

RetCode HosV4L2Dev::QueueBuffer(const std::string& cameraID,
                                const std::shared_ptr<FrameSpec>& frameSpec)
{
    int rc = 0;
    int fd = 0;

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("QueueBuffer: GetCurrentFd error\n");
        return RC_ERROR;
    }

    if (frameSpec == nullptr || myBuffers_ == nullptr) {
        CAMERA_LOGE(" QueueBuffer frameSpec or myBuffers_ is NULL\n");
        return RC_ERROR;
    }

    rc = myBuffers_->V4L2QueueBuffer(fd, frameSpec);
    if (rc == RC_ERROR) {
        CAMERA_LOGE("QueueBuffer: V4L2QueueBuffer error\n");
        return RC_ERROR;
    }
    currStartQueueBuffNum++;
    CAMERA_LOGD("QueueBuffer: currStartQueueBuffNum=%{public}d\n",
                currStartQueueBuffNum.load());
    return RC_OK;
}

RetCode HosV4L2Dev::ReleaseBuffers(const std::string& cameraID)
{
    int fd = 0;
    int rc = 0;

    if (myBuffers_ == nullptr) {
        CAMERA_LOGE("ReleaseBuffers myBuffers_ is NULL\n");
        return RC_ERROR;
    }

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("ReleaseBuffers: GetCurrentFd error\n");
        return RC_ERROR;
    }

    rc = myBuffers_->V4L2ReleaseBuffers(fd);
    if (rc == RC_ERROR) {
        CAMERA_LOGE("ReleaseBuffers: V4L2ReleaseBuffers error\n");
        return RC_ERROR;
    }

    return RC_OK;
}

void HosV4L2Dev::loopBuffers()
{
    int nfds = 0;
    int rc = 0;
    CAMERA_LOGD("!!! loopBuffers enter\n");
    prctl(PR_SET_NAME, "v4l2_loopbuffer");

    CAMERA_LOGD("loopBuffers: poll streamNumber_  = %{public}d\n", streamNumber_.load());
    while (streamNumber_ > 0) {
        if (myBuffers_ == nullptr) {
            CAMERA_LOGE("myBuffers_ is nullptr");
            return;
        }
        while (currStartQueueBuffNum > 0) {
            rc = myBuffers_->V4L2DequeueBuffer(fds_[0].fd);
            if (rc == RC_ERROR) {
                CAMERA_LOGE("loopBuffers: myBuffers_->V4L2DequeueBuffer return error == %{public}d\n", rc);
            }
            CAMERA_LOGD("loopBuffers: currStartQueueBuffNum=%{public}d, streamNumber_=%{public}d,\n",
                currStartQueueBuffNum.load(), streamNumber_.load());
            currStartQueueBuffNum--;
        }
    }
    const int DELAYTIME = 2000;
    usleep(DELAYTIME);
    while (currStartQueueBuffNum > 0) {
        rc = myBuffers_->V4L2DequeueBuffer(fds_[0].fd);
        if (rc == RC_ERROR) {
            CAMERA_LOGE("loopBuffers: myBuffers_->V4L2DequeueBuffer 1 return error == %{public}d\n", rc);
        }
        CAMERA_LOGD("loopBuffers1: currStartQueueBuffNum=%{public}d, streamNumber_=%{public}d,\n",
            currStartQueueBuffNum.load(), streamNumber_.load());
        currStartQueueBuffNum--;
    }
    CAMERA_LOGD("!!! loopBuffers exit\n");
}

RetCode HosV4L2Dev::CreateEpoll(int fd, const unsigned int streamNumber)
{
    if (streamNumber == 0) {
        std::lock_guard<std::mutex> l(pollLock_);
        for (int i = 0; i < MAXSTREAMCOUNT; i++) {
            fds_[i].fd = -1;
            fds_[i].events = 0;
            fds_[i].revents = 0;
        }
        fds_[0].fd = fd;
        fds_[0].events = POLLIN;
        fdNum_ = 1;
    } else {
        std::lock_guard<std::mutex> l(pollLock_);
        if (fd == fds_[0].fd) {
            return RC_OK;
        } else {
            fds_[0].fd = fd;
            fds_[0].events = POLLIN;
            fdNum_ = 1;
        }
    }
    return RC_OK;
}

void HosV4L2Dev::EraseEpoll(int fd)
{
    if (streamNumber_ >= 0) {
        std::lock_guard<std::mutex> l(pollLock_);
        (void)memset_s(fds_, sizeof(fds_), 0, sizeof(fds_));
        fdNum_ = 0;
    }
}

RetCode HosV4L2Dev::StartStream(const std::string& cameraID)
{
    int rc = 0;
    int fd = 0;

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("error: ReqBuffers: GetCurrentFd error\n");
        return RC_ERROR;
    }

    if (myStreams_ == nullptr) {
        myStreams_ = std::make_shared<HosV4L2Streams>(bufferType_);
        if (myStreams_ == nullptr) {
            CAMERA_LOGE("error: StartStream: myStreams_ make_shared is NULL\n");
            return RC_ERROR;
        }
    }

    rc = myStreams_->V4L2StreamOn(fd);
    if (rc == RC_ERROR) {
        CAMERA_LOGE("error: StartStream: V4L2StreamOn error\n");
        return RC_ERROR;
    }

    rc = CreateEpoll(fd, streamNumber_);
    if (rc == RC_ERROR) {
        CAMERA_LOGE("StartStream: CreateEpoll error\n");
        return RC_ERROR;
    }
    currStartQueueBuffNum = 0;
    if (streamNumber_ == 0) {
        streamNumber_++;
        CAMERA_LOGD(
            "StartStream: currStartQueueBuffNum=%{public}d, "
            "streamNumber_=%{public}d,\n",
            currStartQueueBuffNum.load(), streamNumber_.load());
        streamThread_ =
            new (std::nothrow) std::thread(&HosV4L2Dev::loopBuffers, this);
        if (streamThread_ == nullptr) {
            streamNumber_--;
            CAMERA_LOGE("V4L2 StartStream start thread failed\n");
            return RC_ERROR;
        }
    }
    return RC_OK;
}

RetCode HosV4L2Dev::StopStream(const std::string& cameraID)
{
    int rc = 0;
    int fd = 0;

    if (myStreams_ == nullptr) {
        CAMERA_LOGE("error: StopStream: myStreams_ is NULL\n");
        return RC_ERROR;
    }

    if (streamThread_ == nullptr) {
        CAMERA_LOGE("StopStream thread is stopped\n");
        return RC_ERROR;
    }

    streamNumber_ -= 1;
    CAMERA_LOGD("HosV4L2Dev::StopStream streamNumber_ = %{public}d\n",
                streamNumber_.load());

    if (streamNumber_ == 0) {
        CAMERA_LOGD("waiting loopBuffers stop\n");
        streamThread_->join();
        CAMERA_LOGD("loopBuffers streamThread_ stop\n");
    }

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("error: StopStream: GetCurrentFd error\n");
        return RC_ERROR;
    }

    rc = myStreams_->V4L2StreamOff(fd);
    if (rc == RC_ERROR) {
        CAMERA_LOGE("error: StopStream: V4L2StreamOn error\n");
        return RC_ERROR;
    }

    EraseEpoll(fd);

    if (streamNumber_ == 0) {
        delete streamThread_;
        streamThread_ = nullptr;
    }

    return RC_OK;
}

RetCode HosV4L2Dev::ProbeStreamOn(const std::string& cameraId)
{
    CAMERA_LOGI("ProbeStreamOn enter, cameraID = %{public}s", cameraId.c_str());
    int fd = GetCurrentFd(cameraId);
    if (fd < 0) {
        CAMERA_LOGE("ProbeStreamOn: GetCurrentFd error");
        return RC_ERROR;
    }
    if (myStreams_ == nullptr) {
        myStreams_ = std::make_shared<HosV4L2Streams>(bufferType_);
        if (myStreams_ == nullptr) {
            CAMERA_LOGE("ProbeStreamOn: myStreams_ make_shared failed");
            return RC_ERROR;
        }
    }
    RetCode rc = myStreams_->V4L2StreamOn(fd);
    if (rc == RC_OK) {
        myStreams_->V4L2StreamOff(fd);
        CAMERA_LOGI("ProbeStreamOn success, cameraID = %{public}s", cameraId.c_str());
    } else {
        CAMERA_LOGE("ProbeStreamOn failed, cameraID = %{public}s", cameraId.c_str());
    }
    return rc;
}

RetCode HosV4L2Dev::FlashControl(const char* cmd, int cmd_len)
{
    CAMERA_LOGI("HosV4L2Dev::FlashControl %{public}s", cmd);
    int fd = open("/sys/class/misc/sprd_flash/test", O_WRONLY | O_NONBLOCK, 0);
    if (fd < 0) {
        CAMERA_LOGE("demo test:open flash file error %{public}s.....\n",
                    strerror(errno));
        return RC_ERROR;
    }

    int len = write(fd, cmd, cmd_len);
    if (len < 1) {
        close(fd);
        CAMERA_LOGE("demo test:open flash file error %{public}s.....\n",
                    strerror(errno));
        return RC_ERROR;
    }

    close(fd);
    CAMERA_LOGI("HosV4L2Dev::FlashControl %{public}s success.", cmd);
    return RC_OK;
}

RetCode HosV4L2Dev::UpdateSetting(const std::string& cameraID, AdapterCmd command, const int* args)
{
    int32_t fd;
    int rc = 0;

    if (myControl_ == nullptr)
        myControl_ = std::make_shared<HosV4L2Control>();

    if ((myControl_ == nullptr) || (args == nullptr))
        return RC_ERROR;

    fd = GetCurrentFd(cameraID);
    if (fd < 0)
        return RC_ERROR;

    switch (command) {
        case CMD_EXPOSURE_MODE:
            rc = myControl_->V4L2SetCtrl(fd, V4L2_CID_EXPOSURE_AUTO, *(int32_t*)args);
            break;
        case CMD_AE_EXPOTIME:
            rc = myControl_->V4L2SetCtrl(fd, V4L2_CID_EXPOSURE_ABSOLUTE, *(int32_t*)args);
            break;
        case CMD_EXPOSURE_COMPENSATION:
            rc = myControl_->V4L2SetCtrl(fd, V4L2_CID_EXPOSURE, *(int32_t*)args);
            break;
        case CMD_AWB_MODE:
            rc = myControl_->V4L2SetCtrl(fd, V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE, *(int32_t*)args);
            break;
        case CMD_METER_MODE:
            rc = myControl_->V4L2SetCtrl(fd, V4L2_CID_EXPOSURE_METERING, *(int32_t*)args);
            break;
        case CMD_FLASH_MODE:
            switch ((int32_t)(args[0])) {
                case 0:
                    FlashControl("0x03", LEN);
                    break;
                case 1:
                    FlashControl("0x02", LEN);
                default:
                    break;
            }
            break;
        case CMD_FOCUS_REGION:
            struct v4l2_rect r;
            r.left = ((int32_t)(args[0]));
            r.top = ((int32_t)(args[1]));
            r.width = RECTREGION;
            r.height = RECTREGION;
            if (HosV4L2Dev::v4l2Handle_->ioctl(fd, VIDIOC_S_INPUT, &r) < 0)
                return RC_ERROR;
        default:
            break;
    }

    return rc;
}

RetCode HosV4L2Dev::QuerySetting(const std::string& cameraID,
                                 unsigned int command, int* args)
{
    int32_t fd = 0;
    int32_t value = 0;
    int rc = 0;

    if (args == nullptr) {
        CAMERA_LOGE("HosV4L2Dev::QuerySetting: args is NULL\n");
        return RC_ERROR;
    }

    if (myControl_ == nullptr) {
        myControl_ = std::make_shared<HosV4L2Control>();
        if (myControl_ == nullptr) {
            CAMERA_LOGE(
                "HosV4L2Dev::QuerySetting: myControl_ make_shared is NULL\n");
            return RC_ERROR;
        }
    }

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("QuerySetting: GetCurrentFd error\n");
        return RC_ERROR;
    }

    rc = myControl_->V4L2GetCtrl(fd, command, value);
    if (rc != RC_OK) {
        return RC_ERROR;
    }

    *(int32_t*)args = value;

    return RC_OK;
}

RetCode HosV4L2Dev::GetNumberCtrls(const std::string& cameraID,
                                   std::vector<DeviceControl>& control)
{
    int32_t fd = 0;

    if (myControl_ == nullptr) {
        myControl_ = std::make_shared<HosV4L2Control>();
        if (myControl_ == nullptr) {
            CAMERA_LOGE(
                "HosV4L2Dev::GetNumberCtrls: myControl_ make_shared is NULL\n");
            return RC_ERROR;
        }
    }

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("GetNumberCtrls: GetCurrentFd error\n");
        return RC_ERROR;
    }

    return myControl_->V4L2GetCtrls(fd, control, control.size());
}

RetCode HosV4L2Dev::SetNumberCtrls(const std::string& cameraID,
                                   std::vector<DeviceControl>& control)
{
    int32_t fd = 0;

    if (myControl_ == nullptr) {
        myControl_ = std::make_shared<HosV4L2Control>();
        if (myControl_ == nullptr) {
            CAMERA_LOGE(
                "HosV4L2Dev::SetNumberCtrls: myControl_ make_shared is NULL\n");
            return RC_ERROR;
        }
    }

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("SetNumberCtrls: GetCurrentFd error\n");
        return RC_ERROR;
    }

    return myControl_->V4L2SetCtrls(fd, control, control.size());
}

RetCode HosV4L2Dev::GetControls(const std::string& cameraID,
                                std::vector<DeviceControl>& control)
{
    int fd = 0;
    int rc = 0;

    if (myControl_ == nullptr) {
        myControl_ = std::make_shared<HosV4L2Control>();
        if (myControl_ == nullptr) {
            CAMERA_LOGE(
                "HosV4L2Dev::GetControls: myControl_ make_shared is NULL\n");
            return RC_ERROR;
        }
    }

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("GetControls: GetCurrentFd error\n");
        return RC_ERROR;
    }

    rc = myControl_->V4L2GetControls(fd, control);
    if (rc == RC_ERROR) {
        CAMERA_LOGE("myControl_->V4L2GetControls fail\n");
        return RC_ERROR;
    }

    return RC_OK;
}

RetCode HosV4L2Dev::GetFmtDescs(const std::string& cameraID,
                                std::vector<DeviceFormat>& fmtDesc)
{
    int fd = 0;
    int rc = 0;

    if (myFileFormat_ == nullptr) {
        CAMERA_LOGE("GetFmtDescs: myFileFormat_ == nullptr\n");
        return RC_ERROR;
    }

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("GetFmtDescs: GetCurrentFd error\n");
        return RC_ERROR;
    }

    rc = myFileFormat_->V4L2GetFmtDescs(fd, fmtDesc);
    if (rc == RC_ERROR) {
        CAMERA_LOGE("myFileFormat_->V4L2GetFmtDescs fail\n");
        return RC_ERROR;
    }

    return RC_OK;
}

RetCode HosV4L2Dev::ConfigFps(const int fd, DeviceFormat& format,
                              V4l2FmtCmd command)
{
    RetCode rc = RC_OK;

    if (myStreams_ == nullptr) {
        myStreams_ = std::make_shared<HosV4L2Streams>(bufferType_);
        if (myStreams_ == nullptr) {
            CAMERA_LOGE(
                "error: ConfigSys: myStreams_ make_shared is nullptr\n");
            return RC_ERROR;
        }
    }

    if (command == CMD_V4L2_SET_FPS) {
        rc = myStreams_->V4L2StreamFPSSet(fd, format);
    } else {
        rc = myStreams_->V4L2StreamFPSGet(fd, format);
    }

    if (rc != RC_OK) {
        CAMERA_LOGE("ConfigFps CMD %{public}d fail\n", command);
    }

    return rc;
}

RetCode HosV4L2Dev::ConfigSys(const std::string& cameraID, V4l2FmtCmd command,
                              DeviceFormat& format)
{
    int fd = 0;
    RetCode rc = RC_OK;

    if (myFileFormat_ == nullptr) {
        CAMERA_LOGE("GetFmtDescs: ConfigSys == nullptr\n");
        return RC_ERROR;
    }

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("ConfigSys: GetCurrentFd error\n");
        return RC_ERROR;
    }

    switch (command) {
        case CMD_V4L2_GET_FORMAT:
            rc = myFileFormat_->V4L2GetFmt(fd, format);
            break;

        case CMD_V4L2_SET_FORMAT:
            rc = myFileFormat_->V4L2SetFmt(fd, format);
            break;

        case CMD_V4L2_GET_CROPCAP:
            rc = myFileFormat_->V4L2GetCropCap(fd, format);
            break;

        case CMD_V4L2_GET_CROP:
            rc = myFileFormat_->V4L2GetCrop(fd, format);
            break;

        case CMD_V4L2_SET_CROP:
            rc = myFileFormat_->V4L2SetCrop(fd, format);
            break;

        case CMD_V4L2_SET_FPS:
        case CMD_V4L2_GET_FPS:
            rc = ConfigFps(fd, format, command);
            break;

        default:
            CAMERA_LOGE("HosV4L2Dev::ConfigSys unknown command\n");
            break;
    }

    if (rc != RC_OK) {
        CAMERA_LOGE("ConfigSys CMD %{public}d fail\n", command);
    }

    return rc;
}

RetCode HosV4L2Dev::SetV4L2DevCallback(BufCallback cb)
{
    if (cb == nullptr) {
        CAMERA_LOGE("HosV4L2Dev::SetCallback is null");
        return RC_ERROR;
    }
    if (myBuffers_ == nullptr) {
        CAMERA_LOGE("SetCallback myBuffers_ is NULL\n");
        return RC_ERROR;
    }

    myBuffers_->SetV4L2BuffersCallback(cb);

    return RC_OK;
}
RetCode HosV4L2Dev::Flush(const std::string& cameraID)
{
    int rc = 0;
    int fd = 0;

    fd = GetCurrentFd(cameraID);
    if (fd < 0) {
        CAMERA_LOGE("HosV4L2Dev::Flush: GetCurrentFd error\n");
        return RC_ERROR;
    }

    if (myBuffers_ == nullptr) {
        CAMERA_LOGE(" HosV4L2Dev::Flush myBuffers_ is NULL\n");
        return RC_ERROR;
    }

    rc = myBuffers_->Flush(fd);
    if (rc == RC_ERROR) {
        CAMERA_LOGE("HosV4L2Dev::Flush: error\n");
        return RC_ERROR;
    }

    return RC_OK;
}

void HosV4L2Dev::SetMemoryType(uint8_t& memType)
{
    CAMERA_LOGD("func[HosV4L2Dev::%{public}s] memType[%{public}d]", __func__,
                memType);
    if (memType == V4L2_MEMORY_MMAP) {
        memoryType_ = V4L2_MEMORY_MMAP;
    } else {
        memoryType_ = V4L2_MEMORY_MMAP;
    }
}
}  // namespace OHOS::Camera
