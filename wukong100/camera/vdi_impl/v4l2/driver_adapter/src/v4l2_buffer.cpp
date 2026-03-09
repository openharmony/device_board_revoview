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

#include "v4l2_buffer.h"

#include <sys/mman.h>

#include "securec.h"
#include "v4l2_dev.h"

namespace OHOS::Camera {
HosV4L2Buffers::HosV4L2Buffers(enum v4l2_memory memType,
                               enum v4l2_buf_type bufferType)
    : memoryType_(memType), bufferType_(bufferType)
{
    adapterBufferMapNum = 0;
}

HosV4L2Buffers::~HosV4L2Buffers() {}

RetCode HosV4L2Buffers::V4L2ReqBuffers(int fd, int unsigned buffCont)
{
    struct v4l2_requestbuffers req = {};

    CAMERA_LOGD("V4L2ReqBuffers buffCont=%{public}d\n", buffCont);

    req.count = buffCont;
    req.type = bufferType_;
    req.memory = memoryType_;

    if (HosV4L2Dev::v4l2Handle_->ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        CAMERA_LOGE("does not support memory mapping %{public}s\n",
                    strerror(errno));
        return RC_ERROR;
    }

    if (req.count != buffCont) {
        CAMERA_LOGE("error Insufficient buffer memory on \n");

        req.count = 0;
        req.type = bufferType_;
        req.memory = memoryType_;
        if (HosV4L2Dev::v4l2Handle_->ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
            CAMERA_LOGE(
                "V4L2ReqBuffers does not release buffer	%{public}s\n",
                strerror(errno));
            return RC_ERROR;
        }

        return RC_ERROR;
    }
    return RC_OK;
}

RetCode HosV4L2Buffers::V4L2QueueBuffer(
    int fd, const std::shared_ptr<FrameSpec>& frameSpec)
{
    struct v4l2_buffer buf = {};
    struct v4l2_plane planes[1] = {};

    if (frameSpec == nullptr) {
        return RC_ERROR;

    if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
        buf.m.planes = planes;

    if (adapterBufferMapNum > 0) {
        int32_t index = (uint32_t)frameSpec->buffer_->GetIndex();

        auto findIf = adapterBufferMap_.find(index);
        if (findIf != adapterBufferMap_.end()) {
            if (findIf->second.userBufPtr == nullptr) {
                findIf->second.userBufPtr = frameSpec->buffer_->GetVirAddress();
                adapterBufferMapNum--;
            }
        }
    }

    MakeInqueueBuffer(buf, frameSpec);

    bufferLock_.lock();
    auto itr = queueBuffers_.find(fd);
    if (itr != queueBuffers_.end()) {
        const int MAXBUFFER = 8;
        while (itr->second.size() >= MAXBUFFER) {
            bufferLock_.unlock();
            const int DELAYTIME = 20000;
            usleep(DELAYTIME);
            bufferLock_.lock();
        }
        int rc = HosV4L2Dev::v4l2Handle_->ioctl(fd, VIDIOC_QBUF, &buf);
        if (rc < 0) {
            bufferLock_.unlock();
            return RC_ERROR;
        }
        itr->second[buf.index] = frameSpec;
        bufferLock_.unlock();
    } else {
        int rc = HosV4L2Dev::v4l2Handle_->ioctl(fd, VIDIOC_QBUF, &buf);
        if (rc < 0) {
            bufferLock_.unlock();
            return RC_ERROR;
        }
        FrameMap frameMap;
        frameMap.insert(std::make_pair(buf.index, frameSpec));
        queueBuffers_.insert(std::make_pair(fd, frameMap));
        bufferLock_.unlock();
    }

    bufCont++;

    return RC_OK;
}

void HosV4L2Buffers::MakeInqueueBuffer(
    struct v4l2_buffer& buf, const std::shared_ptr<FrameSpec>& frameSpec)
{
    CAMERA_LOGD("HosV4L2Buffers::MakeInqueueBuffer in.");

    buf.index = (uint32_t)frameSpec->buffer_->GetIndex();
    buf.type = bufferType_;
    buf.memory = memoryType_;

    switch (memoryType_) {
        case V4L2_MEMORY_MMAP:
            SetMmapInqueueBuffer(buf, frameSpec);
            break;
        case V4L2_MEMORY_USERPTR:
            SetInqueueBuffer(buf, frameSpec);
            break;
        case V4L2_MEMORY_OVERLAY:
            break;
        default:
            CAMERA_LOGE("It can not be happening - incorrect memoryType\n");
            return;
    }
    return;
}

void HosV4L2Buffers::SetInqueueBuffer(
    struct v4l2_buffer& buf, const std::shared_ptr<FrameSpec>& frameSpec)
{
    CAMERA_LOGD("HosV4L2Buffers::SetInqueueBuffer in.");
    if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        buf.m.planes[0].length = frameSpec->buffer_->GetSize();
        buf.m.planes[0].m.userptr =
            (unsigned long)frameSpec->buffer_->GetVirAddress();
        buf.length = 1;
    } else if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
        buf.length = frameSpec->buffer_->GetSize();
        buf.m.userptr = (unsigned long)frameSpec->buffer_->GetVirAddress();
    }
    return;
}

void HosV4L2Buffers::SetMmapInqueueBuffer(
    struct v4l2_buffer& buf, const std::shared_ptr<FrameSpec>& frameSpec)
{
    CAMERA_LOGD("HosV4L2Buffers::SetMmapInqueueBuffer in.");
    if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        buf.m.planes[0].length = adapterBufferMap_[buf.index].length;
        buf.m.planes[0].m.mem_offset = adapterBufferMap_[buf.index].offset;
        buf.length = 1;
    } else if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
        buf.length = adapterBufferMap_[buf.index].length;
        buf.m.offset = adapterBufferMap_[buf.index].offset;
    }
    return;
}

RetCode HosV4L2Buffers::V4L2DequeueBuffer(int fd)
{
    struct v4l2_buffer buf = {};
    struct v4l2_plane planes[1] = {};
    if (bufCont < 1)
        return RC_ERROR;
    buf.type = bufferType_;
    buf.memory = memoryType_;
    CAMERA_LOGD("V4L2DequeueBuffer memoryType_ =  %{public}d\n", (int)memoryType_);
    if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        buf.m.planes = planes;
        buf.length = 1;
    }
    int rc = HosV4L2Dev::v4l2Handle_->ioctl(fd, VIDIOC_DQBUF, &buf);
    CAMERA_LOGD("V4L2DequeueBuffer memoryType_  buf.index == %{public}d rc=%{public}d\n", buf.index, rc);
    if (rc < 0) {
        CAMERA_LOGE("HosV4L2Dev::v4l2Handle_->ioctl VIDIOC_DQBUF failed: %{public}s\n", strerror(errno));
        return RC_ERROR;
    }
    if (memoryType_ == V4L2_MEMORY_MMAP) {
        if (adapterBufferMap_[buf.index].userBufPtr && adapterBufferMap_[buf.index].start) {
            if (memcpy_s(adapterBufferMap_[buf.index].userBufPtr, adapterBufferMap_[buf.index].length,
                         adapterBufferMap_[buf.index].start, adapterBufferMap_[buf.index].length) != 0) {
                rc = RC_ERROR;
            }
        } else {
            rc = RC_ERROR;
        }
    } else {
        rc = RC_ERROR;
    }
    bufferLock_.lock();
    auto IterMap = queueBuffers_.find(fd);
    if (IterMap == queueBuffers_.end()) {
        bufferLock_.unlock();
        CAMERA_LOGE("std::map queueBuffers_ no fd\n");
        return RC_ERROR;
    }
    auto& bufferMap = IterMap->second;
    auto Iter = bufferMap.find(buf.index);
    if (Iter == bufferMap.end()) {
        bufferLock_.unlock();
        CAMERA_LOGE("V4L2DequeueBuffer buf.index == %{public}d is not find in FrameMap\n", buf.index);
        return RC_ERROR;
    }
    if (dequeueBuffer_ == nullptr) {
        bufferMap.erase(Iter);
        bufferLock_.unlock();
        CAMERA_LOGE("V4L2DequeueBuffer buf.index == %{public}d no callback\n", buf.index);
        return RC_ERROR;
    }
    std::shared_ptr<FrameSpec> framebuff = Iter->second;
    bufferMap.erase(Iter);
    bufferLock_.unlock();
    if (rc < 0) {
        framebuff->buffer_->SetBufferStatus(CAMERA_BUFFER_STATUS_INVALID);
    }
    dequeueBuffer_(framebuff);
    bufCont--;
    CAMERA_LOGD("V4L2DequeueBuffer success bufCont = %{public}d\n", bufCont.load());
    return RC_OK;
}

RetCode HosV4L2Buffers::V4L2AllocBuffer(int fd, int buffcont)
{
    for (int i = 0; i < buffcont; i++) {
        struct v4l2_buffer buf = {};
        struct v4l2_plane planes[1] = {};
        CAMERA_LOGD("V4L2AllocBuffer %{public}d\n", i);

        buf.type = bufferType_;
        buf.memory = memoryType_;
        buf.index = i;
        if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            buf.m.planes = planes;
            buf.length = 1;
        }

        if (HosV4L2Dev::v4l2Handle_->ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            CAMERA_LOGE(
                "error: HosV4L2Dev::v4l2Handle_->ioctl VIDIOC_QUERYBUF "
                "buf.index = "
                "%{public}d,failed: %{public}s\n",
                i, strerror(errno));
            return RC_ERROR;
        }

        CAMERA_LOGD("buf.length = %{public}d buf.index = %{public}d\n",
                    buf.length, buf.index);
        if (buf.length <= 0) {
            CAMERA_LOGE("RROR:user buff V4L2 buf.length failed\n");
            return RC_ERROR;
        }
        if (memoryType_ == V4L2_MEMORY_MMAP) {
            if (SetAdapterBuffer(fd, buf) != RC_OK) {
                return RC_ERROR;
            }
        }
    }
    adapterBufferMapNum = buffcont;
    return RC_OK;
}

RetCode HosV4L2Buffers::V4L2AllocBuffer(
    int fd, const std::shared_ptr<FrameSpec>& frameSpec)
{
    struct v4l2_buffer buf = {};
    struct v4l2_plane planes[1] = {};
    CAMERA_LOGD("V4L2AllocBuffer\n");

    if (frameSpec == nullptr) {
        CAMERA_LOGE("V4L2AllocBuffer frameSpec is NULL\n");
        return RC_ERROR;
    }

    buf.type = bufferType_;
    buf.memory = memoryType_;
    buf.index = (uint32_t)frameSpec->buffer_->GetIndex();
    if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        buf.m.planes = planes;
        buf.length = 1;
    }

    if (HosV4L2Dev::v4l2Handle_->ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        CAMERA_LOGE(
            "error: HosV4L2Dev::v4l2Handle_->ioctl VIDIOC_QUERYBUF failed: "
            "%{public}s\n",
            strerror(errno));
        return RC_ERROR;
    }

    CAMERA_LOGD(
        "buf.length = %{public}d frameSpec->buffer_->GetSize() = %{public}d "
        "buf.index = %{public}d\n",
        buf.length, frameSpec->buffer_->GetSize(), buf.index);
    if (buf.length > frameSpec->buffer_->GetSize()) {
        CAMERA_LOGE("RROR:user buff < V4L2 buf.length\n");
        return RC_ERROR;
    }
    if (memoryType_ == V4L2_MEMORY_MMAP) {
        return SetAdapterBuffer(fd, buf, frameSpec);
    }
    return RC_OK;
}

RetCode HosV4L2Buffers::SetAdapterBuffer(int fd, struct v4l2_buffer& buf)
{
    CAMERA_LOGD("HosV4L2Buffers::SetAdapterBuffer buf.index=%{public}d in.",
                buf.index);
    int32_t ret = 0;
    int32_t index = buf.index;

    auto findIf = adapterBufferMap_.find(index);
    if (findIf == adapterBufferMap_.end()) {
        AdapterBuffer adapterBuffer = {nullptr, 0, 0, nullptr, 0, 0};
        adapterBufferMap_.insert(std::make_pair(index, adapterBuffer));
    }

    switch (memoryType_) {
        case V4L2_MEMORY_MMAP:
            if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
                adapterBufferMap_[index].length = buf.m.planes[0].length;
                adapterBufferMap_[index].offset = buf.m.planes[0].m.mem_offset;
            } else if ((bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE) |
                       (bufferType_ == V4L2_BUF_TYPE_VIDEO_OUTPUT)) {
                adapterBufferMap_[index].length = buf.length;
                adapterBufferMap_[index].offset = buf.m.offset;
            }
            if (adapterBufferMap_[buf.index].start == nullptr) {
                adapterBufferMap_[buf.index].start =
                    HosV4L2Dev::v4l2Handle_->mmap(
                        NULL, adapterBufferMap_[buf.index].length,
                        PROT_READ | PROT_WRITE, bufferType_, fd,
                        adapterBufferMap_[buf.index].offset);
                if (adapterBufferMap_[buf.index].start == MAP_FAILED) {
                    CAMERA_LOGE("SetAdapterBuffer mmap failed.");
                    return RC_ERROR;
                }
            }
            break;

        default:
            CAMERA_LOGE("Incorrect memoryType\n");
            return RC_ERROR;
    }

    CAMERA_LOGD(
        "HosV4L2Buffers::SetAdapterBuffer  buf.index=%{public}d     mazhw-mem2 "
        ".adapterBufferMap_[buf.index].length = %{public}d ",
        buf.index, adapterBufferMap_[buf.index].length);
    return RC_OK;
}

RetCode HosV4L2Buffers::SetAdapterBuffer(
    int fd, struct v4l2_buffer& buf,
    const std::shared_ptr<FrameSpec>& frameSpec)
{
    CAMERA_LOGD("HosV4L2Buffers::SetAdapterBuffer in.");
    int32_t ret = 0;
    int32_t index = (uint32_t)frameSpec->buffer_->GetIndex();

    auto findIf = adapterBufferMap_.find(index);
    if (findIf == adapterBufferMap_.end()) {
        AdapterBuffer adapterBuffer = {nullptr, 0, 0, nullptr, 0, 0};
        adapterBufferMap_.insert(std::make_pair(index, adapterBuffer));
    }

    adapterBufferMap_[index].userBufPtr = frameSpec->buffer_->GetVirAddress();

    switch (memoryType_) {
        case V4L2_MEMORY_MMAP:
            if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
                adapterBufferMap_[index].length = buf.m.planes[0].length;
                adapterBufferMap_[index].offset = buf.m.planes[0].m.mem_offset;
            } else if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
                adapterBufferMap_[index].length = buf.length;
                adapterBufferMap_[index].offset = buf.m.offset;
            }
            if (adapterBufferMap_[buf.index].start == nullptr) {
                adapterBufferMap_[buf.index].start =
                    HosV4L2Dev::v4l2Handle_->mmap(
                        NULL, adapterBufferMap_[buf.index].length,
                        PROT_READ | PROT_WRITE, bufferType_, fd,
                        adapterBufferMap_[buf.index].offset);
                if (adapterBufferMap_[buf.index].start == MAP_FAILED) {
                    CAMERA_LOGE("SetAdapterBuffer mmap failed.");
                    return RC_ERROR;
                }
            }
            break;

        default:
            CAMERA_LOGE("Incorrect memoryType\n");
            return RC_ERROR;
    }
    CAMERA_LOGD("HosV4L2Buffers::SetAdapterBuffer out.");
    return RC_OK;
}

RetCode HosV4L2Buffers::V4L2ReleaseBuffers(int fd)
{
    CAMERA_LOGE("HosV4L2Buffers::V4L2ReleaseBuffers\n");

    bufferLock_.lock();
    queueBuffers_.erase(fd);
    bufferLock_.unlock();
    for (auto& mem : adapterBufferMap_) {
        if (mem.second.start) {
            if (HosV4L2Dev::v4l2Handle_->munmap(mem.second.start,
                                                mem.second.length) < 0) {
            }
        }
    }
    adapterBufferMap_.clear();
    return V4L2ReqBuffers(fd, 0);
}

void HosV4L2Buffers::SetV4L2BuffersCallback(BufCallback cb)
{
    CAMERA_LOGD("HosV4L2Buffers::SetV4L2BuffersCallback OK.");
    dequeueBuffer_ = cb;
}

RetCode HosV4L2Buffers::Flush(int fd)
{
    CAMERA_LOGD("HosV4L2Buffers::Flush\n");
    return RC_OK;
}

void HosV4L2Buffers::ReSetBufCont() { bufCont = 0; }
}  // namespace OHOS::Camera
