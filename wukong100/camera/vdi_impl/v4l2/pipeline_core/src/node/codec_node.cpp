/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "codec_node.h"

#include <dlfcn.h>
#include <securec.h>

#include <ctime>

#include "vdi_types.h"

namespace OHOS::Camera {
void* CodecNode::jpeghandler = DlOpenJpegWrapperLib();
JpegEncodecFunc* CodecNode::jpegencodecfunc = DlDlsymJpegEncodecFunc();
AvcH264EncInterface* CodecNode::h264Encoder = nullptr;
void* CodecNode::h264Handle = nullptr;

AvcH264EncInterface* CodecNode::CreateAvcH264EncInterface()
{
    if (h264Handle == nullptr) {
        void* handler =
            dlopen("/vendor/lib64/libsprd_h264enc_wrapper.z.so", RTLD_LAZY);
        if (handler == nullptr) {
            CAMERA_LOGE(
                "failed to dlopen  /vendor/lib64/libsprd_h264enc_wrapper.z.so, "
                "%{public}s",
                dlerror());
            return nullptr;
        }
        h264Handle = handler;
    }

    typedef AvcH264EncInterface* CreateInstanceFunc();
    CreateInstanceFunc* createInstance =
        (CreateInstanceFunc*)dlsym(h264Handle, "CreateAvcH264Encoder");
    if (createInstance == nullptr) {
        const char* dlsymError = dlerror();
        if (dlsymError) {
            CAMERA_LOGE(
                "Cannot load symbol CreateAvcH264Encoder, %{public}s, "
                "%{public}p",
                dlsymError, createInstance);
        }
        CAMERA_LOGD("load symbol CreateAvcH264Encoder, %{public}p",
                    createInstance);
        return nullptr;
    }

    AvcH264EncInterface* h264EncHandle = createInstance();
    if (h264EncHandle == nullptr) {
        CAMERA_LOGE("CreateAvcH264EncAdapterInstance, failed");
    }

    return h264EncHandle;
}

CodecNode::CodecNode(const std::string& name, const std::string& type,
                     const std::string& cameraId)
    : NodeBase(name, type, cameraId)
{
    if (bufferRotate_ != nullptr) {
        free(bufferRotate_);
        bufferRotate_ = nullptr;
    }
    if (h264Encoder == nullptr) {
        h264Encoder = CodecNode::CreateAvcH264EncInterface();
    }
    CAMERA_LOGV("%{public}s enter, type(%{public}s)\n", name_.c_str(),
                type_.c_str());
}

CodecNode::~CodecNode()
{
    if (bufferRotate_ != nullptr) {
        free(bufferRotate_);
        bufferRotate_ = nullptr;
    }

    startflag = 0;
    CAMERA_LOGI("~CodecNode Node exit.");
}

RetCode CodecNode::Start(const int32_t streamId)
{
    CAMERA_LOGI("RKCodecNode::Start streamId = %{public}d\n", streamId);
    uint64_t bufferPoolId = 0;
    startflag = 0;

    outPutPorts_ = GetOutPorts();
    for (auto& out : outPutPorts_) {
        bufferPoolId = out->format_.bufferPoolId_;
    }

    BufferManager* bufferManager = Camera::BufferManager::GetInstance();
    if (bufferManager == nullptr) {
        CAMERA_LOGE("scale buffer get instance failed");
        return RC_ERROR;
    }

    bufferPool_ = bufferManager->GetBufferPool(bufferPoolId);
    if (bufferPool_ == nullptr) {
        CAMERA_LOGE("get bufferpool failed: %{public}zu", bufferPoolId);
        return RC_ERROR;
    }
    if (bufferRotate_ != nullptr) {
        free(bufferRotate_);
        bufferRotate_ = nullptr;
    }
    return RC_OK;
}

RetCode CodecNode::Stop(const int32_t streamId)
{
    CAMERA_LOGI("CodecNode::Stop streamId = %{public}d\n", streamId);
    startflag = 0;
    if (bufferRotate_ != nullptr) {
        free(bufferRotate_);
        bufferRotate_ = nullptr;
    }
    return RC_OK;
}

RetCode CodecNode::Flush(const int32_t streamId)
{
    CAMERA_LOGI("CodecNode::Flush streamId = %{public}d\n", streamId);
    return RC_OK;
}

unsigned char CodecNode::Clip(int value)
{
    const int bytemaxval = 255;
    return static_cast<unsigned char>(
        value > bytemaxval ? bytemaxval : value < 0 ? 0 : value);
}

void CodecNode::YUVToRGB(int y, int u, int v, unsigned char* red,
                         unsigned char* green, unsigned char* blue,
                         unsigned char* alapha)
{
    const int yConst = 16;
    const int uvConst = 128;
    const double yCoefficient = 1.164;
    const double ruvCoefficient = 2.018;
    const double gvuCoefficient = 0.813;
    const double guvCoefficient = 0.391;
    const double bvuCoefficient = 1.596;
    const int aAlapha = 255;
    int r = yCoefficient * (y - yConst) + ruvCoefficient * (u - uvConst);
    int g = yCoefficient * (y - yConst) + gvuCoefficient * (v - uvConst) -
            guvCoefficient * (u - uvConst);
    int b = yCoefficient * (y - yConst) + bvuCoefficient * (v - uvConst);
    *red = Clip(r);
    *green = Clip(g);
    *blue = Clip(b);
    *alapha = aAlapha;
};

static void Yuv420RotRight90(u_char* dst, u_char* src, int width,
                                int height)
{
    const int interval = 2;
    int size = width * height;
    int pos = 0;
    int n = 0;

    for (int j = 0; j < width; j++) {
        pos = size;
        for (int i = 0; i < height; i++) {
            pos -= width;
            dst[n] = src[pos + j];
            n++;
        }
    }

    int hwidth = width >> 1;
    int hheight = height >> 1;

    u_char* temp = src + size;

    for (int j = 0; j < width; j += interval) {
        pos = width * hheight;
        for (int i = 0; i < hheight; i++) {
            pos -= width;
            dst[n] = temp[pos + j];
            dst[n + 1] = temp[pos + j + 1];
            n += interval;
        }
    }
}

static void Yuv420RotLeft90(u_char* dst, u_char* src, int width,
                               int height)
{
    const int interval = 2;
    int size = width * height;
    int pos = 0;
    int n = 0;

    for (int j = 0; j < width; j++) {
        pos = 0;
        for (int i = 0; i < height; i++) {
            pos += width;
            dst[n] = src[pos - j - 1];
            n++;
        }
    }

    int hwidth = width >> 1;
    int hheight = height >> 1;

    u_char* temp = src + size;

    for (int j = 0; j < width; j += interval) {
        pos = 0;
        for (int i = 0; i < hheight; i++) {
            pos += width;
            dst[n] = temp[pos - j - interval];
            dst[n + 1] = temp[pos - j - 1];
            n += interval;
        }
    }
}

static void Yuv420Rot180(u_char* dst, u_char* src, int width, int height)
{
    const int interval = 2;
    int size = width * height;
    int pos = 0;
    int n = 0;

    pos = size;
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            dst[n] = src[pos - i - 1];
            n++;
        }
        pos -= width;
    }

    int hwidth = width >> 1;
    int hheight = height >> 1;

    u_char* temp = src + size;
    pos = width * hheight;
    for (int j = 0; j < hheight; j++) {
        for (int i = 0; i < width; i += interval) {
            dst[n + 1] = temp[pos - i - 1];
            dst[n] = temp[pos - i - interval];
            n += interval;
        }
        pos -= width;
    }
}

static void Yuv420spRot180(u_char* dst, u_char* src, int width, int height)
{
    const int interval = 2;
    int size = width * height;
    int pos = 0;
    int n = 0;

    pos = size;
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            dst[n] = src[pos - i - 1];
            n++;
        }
        pos -= width;
    }

    int hwidth = width >> 1;
    int hheight = height >> 1;

    u_char* temp = src + size;
    pos = width * hheight;
    for (int j = 0; j < hheight; j++) {
        for (int i = 0; i < width; i += interval) {
            dst[n + 1] = temp[pos - i - 1];
            dst[n] = temp[pos - i - interval];
            n += interval;
        }
        pos -= width;
    }
}

static void Yuv420RotHMirror(u_char* dst, u_char* src, int width,
                               int height)
{
    const int interval = 2;
    int size = width * height;
    int pos = 0;
    int n = 0;

    pos = 0;
    for (int j = 0; j < height; j++) {
        for (int i = width; i > 0; i--) {
            dst[n] = src[pos + i - 1];
            n++;
        }
        pos += width;
    }

    int hwidth = width >> 1;
    int hheight = height >> 1;

    u_char* temp = src + size;
    pos = 0;
    for (int j = 0; j < hheight; j++) {
        for (int i = width; i > 0; i -= interval) {
            dst[n] = temp[pos + i - interval];
            dst[n + 1] = temp[pos + i - 1];
            n += interval;
        }
        pos += width;
    }
}

static void Yuv420RotVMirror(u_char* dst, u_char* src, int width,
                               int height)
{
    const int interval = 2;
    int size = width * height;
    int pos = size;
    int n = 0;

    for (int j = 0; j < height; j++) {
        pos -= width;
        for (int i = 0; i < width; i++) {
            dst[n] = src[pos + i];
            n++;
        }
    }

    int hwidth = width >> 1;
    int hheight = height >> 1;

    u_char* temp = src + size;
    pos = width * hheight;
    for (int j = 0; j < hheight; j++) {
        pos -= width;
        for (int i = 0; i < width; i += interval) {
            dst[n] = temp[pos + i];
            dst[n + 1] = temp[pos + i + 1];
            n += interval;
        }
    }
}

struct CodecMetadataTag {
    std::string cameraId1 = "lcam001";
    CameraId cameraId2 = CAMERA_FIRST;
};

const CodecMetadataTag CODEC_MAP_CAMERA_ID[] = {
    {"lcam001", CAMERA_FIRST}, {"lcam002", CAMERA_SECOND},
    {"lcam003", CAMERA_THIRD}, {"lcam004", CAMERA_FOURTH},
    {"lcam005", CAMERA_FIFTH}, {"lcam006", CAMERA_SIXTH}};

CameraId CodecNode::ConvertCameraId(const std::string& cameraId)
{
    for (auto cameraID : CODEC_MAP_CAMERA_ID) {
        if (cameraID.cameraId1 == cameraId) {
            return cameraID.cameraId2;
        }
    }
    return CAMERA_FIRST;
}

RetCode CodecNode::ConfigJpegOrientation(common_metadata_header_t* data)
{
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_JPEG_ORIENTATION, &entry);
    if (ret != 0 || entry.data.i32 == nullptr) {
        CAMERA_LOGI("tag OHOS_JPEG_ORIENTATION not found");
        return RC_OK;
    }
    const int ROTATION_90 = 90;
    const int ROTATION_180 = 180;
    const int ROTATION_270 = 270;
    int32_t ohosRotation = *entry.data.i32;
    if (ohosRotation == OHOS_CAMERA_JPEG_ROTATION_0) {
        jpegRotation_ = 0;
    } else if (ohosRotation == OHOS_CAMERA_JPEG_ROTATION_90) {
        jpegRotation_ = ROTATION_90;
    } else if (ohosRotation == OHOS_CAMERA_JPEG_ROTATION_180) {
        jpegRotation_ = ROTATION_180;
    } else {
        jpegRotation_ = ROTATION_270;
    }
    return RC_OK;
}

RetCode CodecNode::ConfigJpegQuality(common_metadata_header_t* data)
{
    camera_metadata_item_t entry;
    int ret = FindCameraMetadataItem(data, OHOS_JPEG_QUALITY, &entry);
    if (ret != 0) {
        CAMERA_LOGI("tag OHOS_JPEG_QUALITY not found");
        return RC_OK;
    }

    const int highQualityJpeg = 100;
    const int middleQualityJpeg = 95;
    const int lowQualityJpeg = 85;

    CAMERA_LOGI("OHOS_JPEG_QUALITY is = %{public}d",
                static_cast<int>(entry.data.u8[0]));
    if (*entry.data.i32 == OHOS_CAMERA_JPEG_LEVEL_LOW) {
        jpegQuality_ = lowQualityJpeg;
    } else if (*entry.data.i32 == OHOS_CAMERA_JPEG_LEVEL_MIDDLE) {
        jpegQuality_ = middleQualityJpeg;
    } else if (*entry.data.i32 == OHOS_CAMERA_JPEG_LEVEL_HIGH) {
        jpegQuality_ = highQualityJpeg;
    } else {
        jpegQuality_ = highQualityJpeg;
    }
    return RC_OK;
}

RetCode CodecNode::Config(const int32_t streamId, const CaptureMeta& meta)
{
    (void)streamId;
    if (meta == nullptr) {
        CAMERA_LOGE("meta is nullptr");
        return RC_ERROR;
    }

    common_metadata_header_t* data = meta->get();
    if (data == nullptr) {
        CAMERA_LOGE("data is nullptr");
        return RC_ERROR;
    }

    RetCode rc = ConfigJpegOrientation(data);

    rc = ConfigJpegQuality(data);
    return rc;
}

void* CodecNode::DlOpenJpegWrapperLib()
{
    void* jpeghandlertmp = nullptr;
    CAMERA_LOGD("dlopen /vendor/lib64/libsprd_jpegenc_wrapper.z.so enter");
    jpeghandlertmp =
        dlopen("/vendor/lib64/libsprd_jpegenc_wrapper.z.so", RTLD_LAZY);
    if (jpeghandlertmp == nullptr) {
        CAMERA_LOGE(
            "failed to dlopen  /vendor/lib64/libsprd_jpegenc_wrapper.z.so, "
            "%{public}s",
            dlerror());
        return nullptr;
    }
    CAMERA_LOGD(
        "dlopen /vendor/lib64/libsprd_jpegenc_wrapper.z.so jpeghandler = "
        "%{public}p",
        jpeghandlertmp);
    return jpeghandlertmp;
}

JpegEncodecFunc* CodecNode::DlDlsymJpegEncodecFunc()
{
    if (jpeghandler == nullptr) {
        jpeghandler = DlOpenJpegWrapperLib();
        if (jpeghandler == nullptr) {
            CAMERA_LOGE(
                "failed to dlopen  /vendor/lib64/libsprd_jpegenc_wrapper.z.so, "
                "jpeghandler == nullptr");
            return nullptr;
        }
    }
    JpegEncodecFunc* jpegencodecfunctmp =
        (JpegEncodecFunc*)dlsym(jpeghandler, "UnisocJpegEncodecFunc");
    const char* dlsymError = dlerror();
    if (dlsymError) {
        CAMERA_LOGE("Cannot load symbol UnisocJpegEncodecFunc, %{public}s",
                    dlerror());
        return nullptr;
    }
    CAMERA_LOGD(
        "dlsym UnisocJpegEncodecFunc jpeghandler = %{public}p,jpegencodecfunc= "
        "%{public}p",
        jpeghandler, jpegencodecfunctmp);
    return jpegencodecfunctmp;
}

void CodecNode::Yuv420ToJpegWithUnisoc(std::shared_ptr<IBuffer>& buffer)
{
    if (jpegencodecfunc == nullptr) {
        jpegencodecfunc = DlDlsymJpegEncodecFunc();
        if (jpegencodecfunc == nullptr) {
            CAMERA_LOGD("dlsym UnisocJpegEncodecFunc fail jpeghandler = %{public}p,jpegencodecfunc= %{public}p",
                jpeghandler, jpegencodecfunc);
            buffer->SetBufferStatus(CAMERA_BUFFER_STATUS_INVALID);
            return;
        }
    }
    const int highQualityJpeg = 100;
    struct JpgOpMean mean;
    struct YuvbufFrm src;
    struct YuvbufFrm dst;
    mean.sliceHeight = 0;
    mean.sliceMode = 0;
    mean.isThumb = 0;
    mean.isSync = 1;
    mean.qualityLevel = highQualityJpeg;
    mean.mirror = 0;
    mean.flip = 0;
    mean.rotation = 0;
    CameraId cameraId = ConvertCameraId(cameraId_);
    if (cameraId == CAMERA_FIRST) {
        mean.rotation = 1;
    } else {
        mean.mirror = 0;
        mean.flip = 1;
        mean.rotation = 1;
    }
    src.fmt = JPEGENC_YUV_420;
    uint32_t width = buffer->GetWidth();
    uint32_t height = buffer->GetHeight();
    src.size.width = width;
    src.size.height = height;
    src.addrPhy.addrY = 0;
    src.addrPhy.addrU = width * height;
    src.dataEnd.yEndian = 1;
    const int yuV420UvEndian = 2;
    src.dataEnd.uvEndian = yuV420UvEndian;
    dst.addrPhy.addrY = 0;
    const int yuV420SizeUp = 3;
    const int yuV420SizeDown = 2;
    dst.bufSize = width * height * yuV420SizeUp / yuV420SizeDown;
    if (mean.rotation == 0) {
        dst.size.width = width;
        dst.size.height = height;
    } else {
        dst.size.width = height;
        dst.size.height = width;
    }

    int jpegLength = 0;
    int ret = jpegencodecfunc(mean, src, dst, buffer->GetVirAddress());
    if (ret != 0) {
        buffer->SetSize(jpegLength);
        buffer->SetEsFrameSize(jpegLength);
        CAMERA_LOGE("jpegencodecfunc, ret code = %{public}d", ret);
    } else {
        jpegLength = dst.bufSize;
        if (jpegLength != 0) {
            CAMERA_LOGE("buf_vir 0x%lx size %{public}d\n", dst.addrVir.addrY, jpegLength);
            buffer->SetSize(jpegLength);
            buffer->SetEsFrameSize(jpegLength);
        } else {
            buffer->SetSize(jpegLength);
            buffer->SetEsFrameSize(jpegLength);
            CAMERA_LOGE("jpegLength = 0, error!\n");
        }
    }
}

int CodecNode::Yuv420ToH264WithUnisoc(std::shared_ptr<IBuffer>& buffer,
                                      uint32_t& frameSize)
{
    uint32_t width = buffer->GetWidth();
    uint32_t height = buffer->GetHeight();
    const int yuV420SizeUp = 3;
    const int yuV420SizeDown = 2;
    unsigned int size = width * height * yuV420SizeUp / yuV420SizeDown;
    if (bufferRotate_ == nullptr) {
        bufferRotate_ = (u_char*)malloc(size);
        if (!bufferRotate_) {
            CAMERA_LOGE("malloc memery error!");
            return -1;
        }
    }
    CameraId cameraId = ConvertCameraId(cameraId_);
    if (cameraId == CAMERA_FIRST) {
        CAMERA_LOGE("Do nth for !");
    } else {
        Yuv420RotVMirror(bufferRotate_, (u_char*)buffer->GetVirAddress(),
                           width, height);
        int ret = memcpy_s((void*)buffer->GetVirAddress(), size,
                           (void*)bufferRotate_, size);
        if (ret != 0) {
            CAMERA_LOGE("copy memery error!");
            return -1;
        }
    }
    if (h264Encoder != nullptr) {
        MMInputParams input;
        memset_s(&input, sizeof(input), 0, sizeof(MMInputParams));
        input.orgWidth = 0;
        input.orgHeight = 0;
        input.width = buffer->GetWidth();
        input.height = buffer->GetHeight();
        input.format = H264;
        const int maxframerate = 30;
        input.framerate = maxframerate;
        const int maxkeyinterval = 10;
        input.maxKeyInterval = maxkeyinterval;
        input.cbr = 1;
        const int bitrate = 3000000;
        input.bitrate = bitrate;
        const int qp = 8;
        input.qp = qp;
        input.frames = 0;
        input.eis = 0;
        input.yuvFormat = MMENC_YUV420SP_NV21;
        input.vsp = MMENC_VSP;
        input.fbcMode = FBC_NONE;

        if (input.eis == 0) {
            input.orgHeight = input.height;
            input.orgWidth = input.width;
        }

        CAMERA_LOGI(
            "Yuv420ToH264WithUnisoc:: h264Encoder status is %{public}d, "
            "codecnode "
            "addr is %{public}p",
            status.load(), this);
        if (status == 0) {
            status = 1;
            if (h264Encoder->VspStart(&input, (char*)(buffer->GetVirAddress()),
                                       frameSize) < 0) {
                CAMERA_LOGE("CodecNode:: h264Encoder.VspStart err");
                return -1;
            }
            startflag = 1;
        }
        int type = 0;
        int ret1 = h264Encoder->VspEnc(
            &input, (char*)(buffer->GetVirAddress()), frameSize, type);
        if (ret1 == 0) {
            if (type == 0) {
                buffer->SetEsKeyFrame(1);
                startflag = 0;
            } else {
                if (startflag == 1) {
                    buffer->SetBufferStatus(CAMERA_BUFFER_STATUS_INVALID);
                }
                buffer->SetEsKeyFrame(0);
            }
            CAMERA_LOGE(
                "CodecNode:: h264Encoder.VspStart startflag=%{public}d "
                "key=%{public}d,SetEsKeyFrame=%{public}d",
                startflag, type, buffer->GetEsFrameInfo().isKey);
        } else {
            buffer->SetBufferStatus(CAMERA_BUFFER_STATUS_INVALID);
            buffer->SetEsKeyFrame(0);
        }
        return ret1;
    } else {
        CAMERA_LOGE("CodecNode::Yuv420ToH264WithUnisoc h264Encoder is null");
        return -1;
    }
}

static uint64_t GetPts()
{
    constexpr uint32_t secToNs = 1000000000;
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    uint64_t time = static_cast<uint64_t>(timestamp.tv_sec) * secToNs +
                    static_cast<uint64_t>(timestamp.tv_nsec);
    return time;
}

void CodecNode::DeliverBuffer(std::shared_ptr<IBuffer>& buffer)
{
    if (buffer == nullptr) {
        CAMERA_LOGE("CodecNode::DeliverBuffer frameSpec is null");
        return;
    }

    int32_t id = buffer->GetStreamId();
    CAMERA_LOGD("CodecNode::DeliverBuffer StreamId %{public}d, encode type : %{public}d",
        id, buffer->GetEncodeType());
    if (buffer->GetBufferStatus() != CAMERA_BUFFER_STATUS_OK) {
        CAMERA_LOGE("CodecNode BufferStatus() != CAMERA_BUFFER_STATUS_OK StreamId %{public}d", id);
        return NodeBase::DeliverBuffer(buffer);
    }
    if (buffer->GetEncodeType() == VDI::Camera::V1_0::ENCODE_TYPE_JPEG) {
        Yuv420ToJpegWithUnisoc(buffer);
        CAMERA_LOGI("CodecNode::node codec enter jpeg,%{public}d", (int)buffer->GetSize());
    } else if (buffer->GetEncodeType() == VDI::Camera::V1_0::ENCODE_TYPE_H264) {
        nodestatus = 1;
        uint32_t frameSize = 0;
        Yuv420ToH264WithUnisoc(buffer, frameSize);
        buffer->SetEsFrameSize(frameSize);
        buffer->SetEsTimestamp(GetPts());
    } else {
        CAMERA_LOGI("CodecNode::node buffer format = %{public}d", buffer->GetFormat());
        const int yuV420SizeUp = 3;
        const int yuV420SizeDown = 2;
        int yuvLength = buffer->GetWidth() * buffer->GetHeight() *
                         yuV420SizeUp / yuV420SizeDown;
        buffer->SetEsFrameSize(yuvLength);
        CameraId cameraId = ConvertCameraId(cameraId_);
        if (cameraId != CAMERA_FIRST) {
            if (bufferRotate_ == nullptr) {
                bufferRotate_ = (u_char*)malloc(yuvLength);
                if (!bufferRotate_) {
                    return;
                }
            }
            Yuv420spRot180(bufferRotate_, (u_char*)buffer->GetVirAddress(),
                             buffer->GetWidth(), buffer->GetHeight());
            memcpy_s((void*)buffer->GetVirAddress(), buffer->GetSize(),
                     (void*)bufferRotate_, yuvLength);
        }
    }

    NodeBase::DeliverBuffer(buffer);
}

RetCode CodecNode::Capture(const int32_t streamId, const int32_t captureId)
{
    (void)streamId;
    (void)captureId;
    CAMERA_LOGV("CodecNode::Capture");
    return RC_OK;
}

RetCode CodecNode::CancelCapture(const int32_t streamId)
{
    int ret = RC_OK;
    CAMERA_LOGI("CodecNode::CancelCapture streamid = %{public}d", streamId);
    if (nodestatus == 1) {
        if (status == 1) {
            status = 0;
            CAMERA_LOGI("CodecNode::CancelCapture 1 streamid = %{public}d",
                        streamId);
            if (h264Encoder != nullptr) {
                h264Encoder->VspStop();
            } else {
                CAMERA_LOGE("CodecNode::CancelCapture h264Encoder is null");
                ret = -1;
            }
            CAMERA_LOGI("CodecNode::CancelCapture 2 streamid = %{public}d",
                        streamId);
        }
        nodestatus = 0;
    }
    return ret;
}

REGISTERNODE(CodecNode, {"Codec"})
}  // namespace OHOS::Camera
