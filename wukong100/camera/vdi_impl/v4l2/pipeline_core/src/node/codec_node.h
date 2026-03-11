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

#ifndef HOS_CAMERA_CODEC_NODE_H
#define HOS_CAMERA_CODEC_NODE_H

#include <atomic>
#include <ctime>

#include "avch264encclient.h"
#include "camera.h"
#include "jpeg_api.h"
#include "node_base.h"

namespace OHOS::Camera {

enum VIDEO_RECORD_STATUS { START, ENCODING, STOP };

typedef int JpegEncodecFunc(struct JpgOpMean&, struct YuvbufFrm&,
                            struct YuvbufFrm&, void*);

class CodecNode : public NodeBase {
   public:
    CodecNode(const std::string& name, const std::string& type,
              const std::string& cameraId);
    ~CodecNode() override;
    CameraId ConvertCameraId(const std::string& cameraId);
    RetCode Start(const int32_t streamId) override;
    RetCode Stop(const int32_t streamId) override;
    void DeliverBuffer(std::shared_ptr<IBuffer>& buffer) override;
    virtual RetCode Capture(const int32_t streamId,
                            const int32_t captureId) override;
    RetCode CancelCapture(const int32_t streamId) override;
    RetCode Flush(const int32_t streamId);

    AvcH264EncInterface* CreateAvcH264EncInterface();
    static void* DlOpenJpegWrapperLib();
    static JpegEncodecFunc* DlDlsymJpegEncodecFunc();
    static void* jpeghandler;
    static JpegEncodecFunc* jpegencodecfunc;
    static AvcH264EncInterface* h264Encoder;
    static void* h264Handle;

   private:
    unsigned char Clip(const int value);
    void YUVToRGB(int y, int u, int v, unsigned char* red, unsigned char* green,
                  unsigned char* blue, unsigned char* alapha);
    RetCode ConfigJpegOrientation(common_metadata_header_t* data);
    RetCode ConfigJpegQuality(common_metadata_header_t* data);
    RetCode Config(const int32_t streamId, const CaptureMeta& meta);
    void Yuv420ToJpegWithUnisoc(std::shared_ptr<IBuffer>& buffer);
    int Yuv420ToH264WithUnisoc(std::shared_ptr<IBuffer>& buffer,
                               const uint32_t& frameSize);

    unsigned char* previewTempBuff_ = nullptr;

    std::atomic<unsigned int> status{0};
    int nodestatus = 0;
    std::vector<std::shared_ptr<IPort>> outPutPorts_;
    std::shared_ptr<IBufferPool> bufferPool_ = nullptr;
    uint32_t jpegRotation_;
    uint32_t jpegQuality_;
    u_char* bufferRotate_ = nullptr;
    int startflag = 0;
};
}  // namespace OHOS::Camera
#endif
