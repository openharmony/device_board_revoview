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
#ifndef HOS_CAMERA_V4L2_DEV_H
#define HOS_CAMERA_V4L2_DEV_H

#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include <linux/videodev2.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include "v4l2_buffer.h"
#include "v4l2_fileformat.h"
#include "v4l2_stream.h"
#include "v4l2_control.h"
#include "v4l2_common.h"
#include <atomic>
#if defined(V4L2_UTEST) || defined (V4L2_MAIN_TEST)
#include "v4l2_temp.h"
#else
#include <camera.h>
#include <stream.h>
#endif

namespace OHOS::Camera {
class HosV4L2Dev {
public:
    HosV4L2Dev();
    ~HosV4L2Dev();

    RetCode start(const std::string& cameraID);

    RetCode stop(const std::string& cameraID);

    RetCode SetNumberCtrls (const std::string& cameraID, std::vector<DeviceControl>& control);

    RetCode GetNumberCtrls (const std::string& cameraID, std::vector<DeviceControl>& control);

    RetCode GetFmtDescs(const std::string& cameraID, std::vector<DeviceFormat>& fmtDesc);

    RetCode GetControls(const std::string& cameraID, std::vector<DeviceControl>& control);

    RetCode ConfigSys(const std::string& cameraID, V4l2FmtCmd command, DeviceFormat& format);

    RetCode UpdateSetting(const std::string& cameraID, AdapterCmd command, const int* args);

    RetCode QuerySetting(const std::string& cameraID, unsigned int command, int* args);

    RetCode ReqBuffers(const std::string& cameraID, unsigned int buffCont);

    RetCode CreatBuffer(const std::string& cameraID, const std::shared_ptr<FrameSpec>& frameSpec);

    RetCode CreatBuffer(const std::string& cameraID, int buffcont);

    RetCode DestoryBuffers();

    RetCode StartStream(const std::string& cameraID);

    RetCode QueueBuffer(const std::string& cameraID, const std::shared_ptr<FrameSpec>& frameSpec);

    RetCode ReleaseBuffers(const std::string& cameraID);

    RetCode StopStream(const std::string& cameraID);

    RetCode ProbeStreamOn(const std::string &cameraId);

    RetCode SetV4L2DevCallback(BufCallback cb);

    RetCode Flush(const std::string& cameraID);

    void SetMemoryType(uint8_t &memType);

    RetCode FlashControl(const char* cmd, int len);

    static RetCode Init(std::vector<std::string>& cameraIDs);
    static void Release();

    static std::map<std::string, std::string> deviceMatch;
    static std::map<std::string, int> fdMatch;
    static std::mutex deviceFdLock_;
    static V4l2AdapterInterface *v4l2Handle_;
    static UnisocV4l2ClientFactory global_UnisocV4l2ClientFactory;

    static std::map<std::string, std::string> CreateDevMap()
    {
        std::map<std::string, std::string> tmp_map;
        tmp_map.insert(std::pair<std::string, std::string>("INVALID", "INVALID"));
        return tmp_map;
    }

    static std::map<std::string, int> CreateFdMap()
    {
        std::map<std::string, int> tmp_map;
        tmp_map.insert(std::pair<std::string, int>("INVALID", 0));
        return tmp_map;
    }

    static V4l2AdapterInterface* CreateV4l2AdapterInterface();

private:
    int GetCurrentFd(const std::string& cameraID);
    void loopBuffers();
    RetCode CreateEpoll(int fd, const unsigned int streamNumber);
    void EraseEpoll(int fd);
    RetCode ConfigFps(const int fd, DeviceFormat& format, V4l2FmtCmd command);

    std::thread* streamThread_ = nullptr;
    std::atomic<unsigned int> streamNumber_{0};
    unsigned int fdNum_ = 0;

    struct pollfd fds_[MAXSTREAMCOUNT];
    std::mutex pollLock_;

    std::shared_ptr<HosV4L2Buffers> myBuffers_ = nullptr;
    std::shared_ptr<HosV4L2Streams> myStreams_ = nullptr;
    std::shared_ptr<HosFileFormat> myFileFormat_ = nullptr;
    std::shared_ptr<HosV4L2Control> myControl_ = nullptr;

    enum v4l2_memory memoryType_ = V4L2_MEMORY_MMAP;
    enum v4l2_buf_type bufferType_ = V4L2_BUF_TYPE_PRIVATE;
    std::atomic<int> currStartQueueBuffNum{0};
};
} // namespace OHOS::Camera
#endif // HOS_CAMERA_V4L2_DEV_H
