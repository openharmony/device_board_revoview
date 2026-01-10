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

#ifndef __SPRD_AVCH264ENC_HANDLER_H__
#define __SPRD_AVCH264ENC_HANDLER_H__
#include <cstdio>
#include <cstdint>
#include <string>
#include <memory>
#include <ibuffer.h>

namespace OHOS {
namespace Camera {

typedef enum {
  ITU_H263 = 0,
  MPEG4,
  JPEG,
  FLV_V1,
  H264,
  VP8,
  VP9,
  H265,
  FORMAT_MAX
} VIDEO_STANDARD_E;

typedef enum
{
  MMENC_YUV420P_YU12 = 0,
  MMENC_YUV420P_YV12 = 1,
  MMENC_YUV420SP_NV12 = 2,
  MMENC_YUV420SP_NV21 = 3,
  MMENC_ARGB32 = 4,
  MMENC_RGBA32 = 5,
} MMENC_YUV_FORMAT_E;

typedef enum {
  MMENC_VSP = 0,
  MMENC_VSP_ENC = 1,
} MMENC_VSP_VERSION;

enum FBC_MODE {
  FBC_NONE,
  IFBC = 1,
  AFBC = 2
};

typedef struct {
  uint32_t org_width;
  uint32_t org_height;
  uint32_t width;
  uint32_t height;
  int32_t format;
  uint32_t framerate;
  uint32_t max_key_interval;
  uint32_t cbr;
  uint32_t bitrate;
  uint32_t qp;
  uint32_t frames;
  uint32_t yuv_format;
  uint32_t vsp;
  uint32_t eis;
  uint8_t fbc_mode;
} MMInputParams;

class AvcH264EncInterface {
  public:
    virtual ~AvcH264EncInterface(){};
    virtual int vsp_start(MMInputParams* p_input, char* buffer, uint32_t& frameSize) = 0;
    virtual int vsp_enc(MMInputParams* p_input, char* buffer, uint32_t& frameSize, int& type) = 0;
    virtual int vsp_stop() = 0;
};

class AvcH264EncClient {
  public:
    AvcH264EncClient(const char *name);
    virtual ~AvcH264EncClient() {}

    static AvcH264EncInterface *create();
    static void registerType(AvcH264EncClient *factory);
    const std::string &name() const { return name_; }

  private:
    virtual AvcH264EncInterface *createInstance();
    std::string name_;
};

class UnisocAvcH264EncClientFactory : public AvcH264EncClient {
  public: 
    UnisocAvcH264EncClientFactory() : AvcH264EncClient("UnisocAvcH264EncClient") {};
};
} // namespace Camera
} // namespace OHOS::Camera
#endif /* __SPRD_AVCH264ENC_HANDLER_H__ */