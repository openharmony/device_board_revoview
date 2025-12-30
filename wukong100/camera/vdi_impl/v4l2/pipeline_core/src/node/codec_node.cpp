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
#include <securec.h>

#include <dlfcn.h>
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
        void *handler = dlopen("/vendor/lib64/libsprd_h264enc_wrapper.z.so", RTLD_LAZY);
        if (handler == nullptr) {
            CAMERA_LOGE("failed to dlopen  /vendor/lib64/libsprd_h264enc_wrapper.z.so, %{public}s", dlerror());
            return nullptr;
        }
        h264Handle = handler;
    }

    typedef AvcH264EncInterface* CreateInstanceFunc();
    CreateInstanceFunc* createInstance = (CreateInstanceFunc*) dlsym(h264Handle, "CreateAvcH264Encoder");
    if (createInstance == nullptr) {
        const char* dlsym_error = dlerror();
        if (dlsym_error) {
            CAMERA_LOGE("Cannot load symbol CreateAvcH264Encoder, %{public}s, %{public}p", dlsym_error, createInstance);
        }
        CAMERA_LOGD("load symbol CreateAvcH264Encoder, %{public}p", createInstance);
        return nullptr;
    }
 
    AvcH264EncInterface* H264EncHandle = createInstance();
    if (H264EncHandle == nullptr) {
        CAMERA_LOGE("CreateAvcH264EncAdapterInstance, failed");
    }
 
    return H264EncHandle;
}

CodecNode::CodecNode(const std::string& name, const std::string& type, const std::string &cameraId) : NodeBase(name, type, cameraId)
{
    if (bufferRotate_ != nullptr) {
        free(bufferRotate_) ;
        bufferRotate_ = nullptr;
    }
    if (h264Encoder == nullptr) {
        h264Encoder = CodecNode::CreateAvcH264EncInterface();
    }
    CAMERA_LOGV("%{public}s enter, type(%{public}s)\n", name_.c_str(), type_.c_str());
}

CodecNode::~CodecNode()
{
    if (bufferRotate_ != nullptr) {
        free(bufferRotate_) ;
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
        free(bufferRotate_) ;
        bufferRotate_ = nullptr;
    }
    return RC_OK;
}

RetCode CodecNode::Stop(const int32_t streamId)
{
    CAMERA_LOGI("CodecNode::Stop streamId = %{public}d\n", streamId);
    startflag = 0;
    if (bufferRotate_ != nullptr) {
        free(bufferRotate_) ;
        bufferRotate_ = nullptr;
    }
    return RC_OK;
}

RetCode CodecNode::Flush(const int32_t streamId)
{
    CAMERA_LOGI("CodecNode::Flush streamId = %{public}d\n", streamId);
    return RC_OK;
}

unsigned char CodecNode::Clip(int value) {
    const int BYTEMAXVAL = 255;
    return static_cast<unsigned char>(value > BYTEMAXVAL ? BYTEMAXVAL : value < 0 ? 0 : value);
}

void CodecNode::YUVToRGB(int Y, int U, int V, unsigned char* Red, unsigned char* Green, unsigned char* Blue, unsigned char*Alapha)
{
    const int Y_CONST = 16;
    const int UV_CONST = 128;
    const double Y_COEFFICIENT = 1.164;
    const double RUV_COEFFICIENT = 2.018;
    const double GVU_COEFFICIENT = 0.813;
    const double GUV_COEFFICIENT = 0.391;
    const double BVU_COEFFICIENT = 1.596;
    const int ALAPHA = 255;
    int r = Y_COEFFICIENT * (Y - Y_CONST) + RUV_COEFFICIENT * (U - UV_CONST);
    int g =  Y_COEFFICIENT * (Y - Y_CONST) + GVU_COEFFICIENT * (V - UV_CONST) - GUV_COEFFICIENT * (U - UV_CONST);
    int b = Y_COEFFICIENT * (Y - Y_CONST) + BVU_COEFFICIENT * (V - UV_CONST);
    *Red = Clip(r);
    *Green = Clip(g);
    *Blue = Clip(b);
    *Alapha = ALAPHA;
};

static void Yuv420_Rot_Right_90(u_char* dst, u_char* src, int width, int height)
{
    const int INTERVAL = 2;
    int size = width * height;
    int pos = 0;
    int n = 0;

    for(int j = 0; j < width; j++) {
        pos = size;
        for(int i = 0; i < height; i++) {
            pos -= width;
            dst[n] = src[pos + j];
            n++;
        }
    }

    int hwidth = width >> 1;
    int hheight = height >> 1;

    u_char* temp = src + size;

    for(int j = 0; j < width; j += INTERVAL) {
        pos = width * hheight;
        for(int i = 0; i < hheight; i++) {
            pos -= width;
            dst[n] = temp[pos + j];
            dst[n+1] = temp[pos + j + 1];
            n += INTERVAL;
        }
    }
}

static void Yuv420_Rot_left_90(u_char* dst, u_char* src, int width, int height)
{
    const int INTERVAL = 2;
    int size = width * height;
    int pos = 0;
    int n = 0;

    for(int j = 0; j < width; j++) {
        pos = 0;
        for(int i = 0; i < height; i++) {
            pos += width;
            dst[n] = src[pos - j - 1];
            n++;
        }
    }

    int hwidth = width >> 1;
    int hheight = height >> 1;

    u_char* temp = src + size;

    for(int j = 0; j < width; j += INTERVAL) {
        pos = 0;
        for(int i = 0; i < hheight; i++) {
            pos += width;
            dst[n] = temp[pos - j - INTERVAL];
            dst[n+1] = temp[pos - j - 1];
            n += INTERVAL;
        }
    }
}

static void Yuv420_Rot_180(u_char* dst, u_char* src, int width, int height)
{
    const int INTERVAL = 2;
    int size = width * height;
    int pos = 0;
    int n = 0;

    pos = size;
    for(int j = 0; j < height; j++) {
        for(int i = 0; i < width; i++) {
            dst[n] = src[pos - i - 1];
            n++;
        }
        pos -= width;
    }

    int hwidth = width >> 1;
    int hheight = height >> 1;

    u_char* temp = src + size;
    pos = width * hheight ;
    for(int j = 0; j < hheight; j++) {
        for(int i = 0; i < width; i += INTERVAL) {
            dst[n + 1] = temp[pos - i - 1];
            dst[n] = temp[pos - i - INTERVAL];
            n += INTERVAL;
        }
        pos -= width;
    }
}

static void Yuv420sp_Rot_180(u_char* dst, u_char* src, int width, int height)
{
    const int INTERVAL = 2;
    int size = width * height;
    int pos = 0;
    int n = 0;

    pos = size;
    for(int j = 0; j < height; j++) {
        for(int i = 0; i < width; i++) {
            dst[n] = src[pos - i - 1];
            n++;
        }
        pos -= width;
    }

    int hwidth = width >> 1;
    int hheight = height >> 1;

    u_char* temp = src + size;
    pos = width * hheight ;
    for(int j = 0; j < hheight; j++) {
        for(int i = 0; i < width; i += INTERVAL) {
            dst[n + 1] = temp[pos - i - 1];
            dst[n] = temp[pos - i - INTERVAL];
            n += INTERVAL;
        }
        pos -= width;
    }
}

static void Yuv420_Rot_HMirror(u_char* dst, u_char* src, int width, int height)
{
    const int INTERVAL = 2;
    int size = width * height;
    int pos = 0;
    int n = 0;

    pos = 0;
    for(int j = 0; j < height; j++) {
        for(int i = width; i > 0; i--) {
            dst[n] = src[pos + i - 1];
            n++;
        }
        pos += width;
    }

    int hwidth = width >> 1;
    int hheight = height >> 1;

    u_char* temp = src + size;
    pos = 0;
    for(int j = 0; j < hheight; j++) {
        for(int i = width; i > 0; i -= INTERVAL) {
            dst[n] = temp[pos + i - INTERVAL];
            dst[n+1] = temp[pos + i - 1];
            n += INTERVAL;
        }
        pos += width;
    }
}

static void Yuv420_Rot_VMirror(u_char* dst, u_char* src, int width, int height)
{
    const int INTERVAL = 2;
    int size = width * height;
    int pos = size;
    int n = 0;

    for(int j = 0; j < height; j++) {
        pos -= width;
        for(int i = 0; i < width; i++) {
            dst[n] = src[pos + i];
            n++;
        }
    }

    int hwidth = width >> 1;
    int hheight = height >> 1;

    u_char* temp = src + size;
    pos = width * hheight;
    for(int j = 0; j < hheight; j++) {
        pos -= width;
        for(int i = 0; i < width; i += INTERVAL) {
            dst[n] = temp[pos + i];
            dst[n + 1] = temp[pos + i + 1];
            n += INTERVAL;
        }
    }
}

struct CodecMetadataTag {
    std::string cameraId1 = "lcam001";
    CameraId cameraId2 = CAMERA_FIRST;
};

const CodecMetadataTag g_codecMapCameraId[] = {
    { "lcam001", CAMERA_FIRST },
    { "lcam002", CAMERA_SECOND },
    { "lcam003", CAMERA_THIRD },
    { "lcam004", CAMERA_FOURTH },
    { "lcam005", CAMERA_FIFTH },
    { "lcam006", CAMERA_SIXTH }
};

CameraId CodecNode::ConvertCameraId(const std::string &cameraId)
{
    for (auto cameraID : g_codecMapCameraId) {
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

    const int HIGH_QUALITY_JPEG = 100;
    const int MIDDLE_QUALITY_JPEG = 95;
    const int LOW_QUALITY_JPEG = 85;

    CAMERA_LOGI("OHOS_JPEG_QUALITY is = %{public}d", static_cast<int>(entry.data.u8[0]));
    if (*entry.data.i32 == OHOS_CAMERA_JPEG_LEVEL_LOW) {
        jpegQuality_ = LOW_QUALITY_JPEG;
    } else if (*entry.data.i32 == OHOS_CAMERA_JPEG_LEVEL_MIDDLE) {
        jpegQuality_ = MIDDLE_QUALITY_JPEG;
    } else if (*entry.data.i32 == OHOS_CAMERA_JPEG_LEVEL_HIGH) {
        jpegQuality_ = HIGH_QUALITY_JPEG;
    } else {
        jpegQuality_ = HIGH_QUALITY_JPEG;
    }
    return RC_OK;
}

RetCode CodecNode::Config(const int32_t streamId, const CaptureMeta& meta)
{
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
    jpeghandlertmp = dlopen("/vendor/lib64/libsprd_jpegenc_wrapper.z.so", RTLD_LAZY);
    if (jpeghandlertmp == nullptr) {
        CAMERA_LOGE("failed to dlopen  /vendor/lib64/libsprd_jpegenc_wrapper.z.so, %{public}s", dlerror());
        return nullptr;
    }
    CAMERA_LOGD("dlopen /vendor/lib64/libsprd_jpegenc_wrapper.z.so jpeghandler = %{public}p", jpeghandlertmp);
    return jpeghandlertmp;
}

JpegEncodecFunc* CodecNode::DlDlsymJpegEncodecFunc()
{
    if (jpeghandler == nullptr) {
        jpeghandler = DlOpenJpegWrapperLib();
        if (jpeghandler == nullptr) {
            CAMERA_LOGE("failed to dlopen  /vendor/lib64/libsprd_jpegenc_wrapper.z.so, jpeghandler == nullptr");
            return nullptr;
        }
    }
    JpegEncodecFunc* jpegencodecfunctmp = (JpegEncodecFunc*) dlsym(jpeghandler, "UnisocJpegEncodecFunc");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        CAMERA_LOGE("Cannot load symbol UnisocJpegEncodecFunc, %{public}s", dlerror());
        return nullptr;
    }
    CAMERA_LOGD("dlsym UnisocJpegEncodecFunc jpeghandler = %{public}p,jpegencodecfunc= %{public}p", jpeghandler, jpegencodecfunctmp);
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
    const int HIGH_QUALITY_JPEG = 100;
    struct jpg_op_mean mean;
    struct yuvbuf_frm src;
    struct yuvbuf_frm dst;
    mean.slice_height = 0 ;
    mean.slice_mode = 0;
    mean.is_thumb = 0;
    mean.is_sync = 1;
    mean.quality_level = HIGH_QUALITY_JPEG;
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
    src.addr_phy.addr_y = 0;
    src.addr_phy.addr_u = width * height;
    src.data_end.y_endian = 1;
    const int YUV420_UV_ENDIAN = 2;
    src.data_end.uv_endian= YUV420_UV_ENDIAN;
    dst.addr_phy.addr_y =  0;
    const int YUV420_SIZE_UP = 3;
    const int YUV420_SIZE_DOWN = 2;
    dst.buf_size = width * height * YUV420_SIZE_UP / YUV420_SIZE_DOWN;
    if(mean.rotation == 0)
    {
        dst.size.width = width;
        dst.size.height = height;
    } else {
        dst.size.width = height;
        dst.size.height = width;
    }

    int jpeg_length = 0;
    int ret = jpegencodecfunc(mean, src, dst, buffer->GetVirAddress());
    if (ret != 0) {
        buffer->SetSize(jpeg_length);
        buffer->SetEsFrameSize(jpeg_length);
        CAMERA_LOGE("jpegencodecfunc, ret code = %{public}d", ret);
    } else {
        jpeg_length = dst.buf_size;
        if(jpeg_length != 0)
        {
            CAMERA_LOGE("buf_vir 0x%lx size %{public}d\n", dst.addr_vir.addr_y,jpeg_length);
            buffer->SetSize(jpeg_length);
            buffer->SetEsFrameSize(jpeg_length);
        } else {
            buffer->SetSize(jpeg_length);
            buffer->SetEsFrameSize(jpeg_length);
            CAMERA_LOGE("jpeg_length = 0, error!\n");
        }
    }
}

int CodecNode::Yuv420ToH264WithUnisoc(std::shared_ptr<IBuffer>& buffer, uint32_t& frameSize)
{
    uint32_t width = buffer->GetWidth();
    uint32_t height = buffer->GetHeight();
    const int YUV420_SIZE_UP = 3;
    const int YUV420_SIZE_DOWN = 2;
    unsigned int size = width * height * YUV420_SIZE_UP / YUV420_SIZE_DOWN;
    if (bufferRotate_ == nullptr) {
        bufferRotate_ = (u_char* )malloc(size);
        if(!bufferRotate_) {
            CAMERA_LOGE("malloc memery error!");
            return -1;
        }
    }
    CameraId cameraId = ConvertCameraId(cameraId_);
    if (cameraId == CAMERA_FIRST) {
        Yuv420_Rot_180(bufferRotate_, (u_char*)buffer->GetVirAddress(), width, height);
    } else {
        Yuv420_Rot_VMirror(bufferRotate_, (u_char*)buffer->GetVirAddress(), width, height);
    }
    int ret = memcpy_s((void*)buffer->GetVirAddress(), size, (void*)bufferRotate_, size);
    if (ret != 0) {
        CAMERA_LOGE("copy memery error!");
        return -1;
    }

    if (h264Encoder != nullptr) {
        MMInputParams input;
        memset(&input, 0, sizeof(MMInputParams));
        input.org_width = 0;
        input.org_height = 0;
        input.width = buffer->GetWidth();
        input.height = buffer->GetHeight();
        input.format = H264;
        const int MAXFRAMERATE = 30;
        input.framerate = MAXFRAMERATE;
        const int MAXKEYINTERVAL = 10;
        input.max_key_interval = MAXKEYINTERVAL;
        input.cbr = 1;
        const int BITRATE = 3000000;
        input.bitrate = BITRATE;
        const int QP = 8;
        input.qp = QP;
        input.frames = 0;
        input.eis = 0;
        input.yuv_format = MMENC_YUV420SP_NV21;
        input.vsp = MMENC_VSP;
        input.fbc_mode = FBC_NONE;

        if (input.eis == 0) {
            input.org_height = input.height;
            input.org_width = input.width;
        }

        CAMERA_LOGI("Yuv420ToH264WithUnisoc:: h264Encoder status is %{public}d, codecnode addr is %{public}p", status.load(), this);
        if (status == 0) {
            status = 1;
            if (h264Encoder->vsp_start(&input, (char*)(buffer->GetVirAddress()), frameSize) < 0) {
                CAMERA_LOGE("CodecNode:: h264Encoder.vsp_start err");
                return -1;
            }
            startflag = 1;
        }
        int type = 0;
        int ret1 = h264Encoder->vsp_enc(&input, (char*)(buffer->GetVirAddress()), frameSize, type);
        if (ret1 == 0) {
            if (type == 0) {
                buffer->SetEsKeyFrame(1);
                startflag = 0;
            } else {
                if(startflag == 1) {
                    buffer->SetBufferStatus(CAMERA_BUFFER_STATUS_INVALID);
                }
                buffer->SetEsKeyFrame(0);
            }
            CAMERA_LOGE("CodecNode:: h264Encoder.vsp_start startflag=%{public}d key=%{public}d,SetEsKeyFrame=%{public}d", startflag, type, buffer->GetEsFrameInfo().isKey);
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
    constexpr uint32_t SEC_TO_NS = 1000000000;
    struct timespec timestamp = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    uint64_t time = static_cast<uint64_t>(timestamp.tv_sec) * SEC_TO_NS + static_cast<uint64_t>(timestamp.tv_nsec);
    return time;
}

void CodecNode::DeliverBuffer(std::shared_ptr<IBuffer>& buffer)
{
    if (buffer == nullptr) {
        CAMERA_LOGE("CodecNode::DeliverBuffer frameSpec is null");
        return;
    }

    int32_t id = buffer->GetStreamId();
    CAMERA_LOGD("CodecNode::DeliverBuffer StreamId %{public}d, encode type : %{public}d", id, buffer->GetEncodeType());
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
        const int YUV420_SIZE_UP = 3;
        const int YUV420_SIZE_DOWN = 2;
        int yuv_length = buffer->GetWidth() * buffer->GetHeight() * YUV420_SIZE_UP / YUV420_SIZE_DOWN;
        buffer->SetEsFrameSize(yuv_length);
        CameraId cameraId = ConvertCameraId(cameraId_);
        if (cameraId != CAMERA_FIRST) {
            if (bufferRotate_ == nullptr) {
                bufferRotate_ = (u_char* )malloc(yuv_length);
                if(!bufferRotate_) {
                    return;
                }
            }
            Yuv420sp_Rot_180(bufferRotate_, (u_char*)buffer->GetVirAddress(), buffer->GetWidth(), buffer->GetHeight());
            memcpy_s((void*)buffer->GetVirAddress(), buffer->GetSize(), (void*)bufferRotate_, yuv_length);
        }
    }

    NodeBase::DeliverBuffer(buffer);
}

RetCode CodecNode::Capture(const int32_t streamId, const int32_t captureId)
{
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
            CAMERA_LOGI("CodecNode::CancelCapture 1 streamid = %{public}d", streamId);
            if (h264Encoder != nullptr) {
                h264Encoder->vsp_stop();
            } else {
                CAMERA_LOGE("CodecNode::CancelCapture h264Encoder is null");
                ret = -1;
            }
            CAMERA_LOGI("CodecNode::CancelCapture 2 streamid = %{public}d", streamId);
        }
        nodestatus = 0;
    }
    return ret;
}

REGISTERNODE(CodecNode, {"Codec"})
} // namespace OHOS::Camera
