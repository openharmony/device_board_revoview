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

#ifndef JPEG_API_H
#define JPEG_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long jpg_uint;
typedef long jpg_int;
typedef uint64_t jpg_u64;
typedef int64_t jpg_s64;
typedef unsigned int jpg_u32;
typedef int jpg_s32;
typedef unsigned short jpg_u16;
typedef short jpg_s16;
typedef unsigned char jpg_u8;
typedef char jpg_s8;
typedef void* jpg_handle;

typedef struct JpegCodecCallerHandle {
    void* jpegCx;
    void* reserved;
} JPEG_CODEC_CALLER_T;

typedef enum {
    JPEGENC_YUV_420 = 0,
    JPEGENC_YUV_422,
    JPEGENC_YUV_MAX
} JPEGENC_FORMAT_E;

typedef enum {
    JPEGENC_QUALITY_LOW = 0,
    JPEGENC_QUALITY_MIDDLE_LOW,
    JPEGENC_QUALITY_MIDDLE,
    JPEGENC_QUALITY_MIDDLE_HIGH,
    JPEGENC_QUALITY_HIGH,
    JPEGENC_QUALITY_MAX
} JPEGENC_QUALITY_E;

struct YuvbufRect {
    jpg_u32 startX;
    jpg_u32 startY;
    jpg_u32 width;
    jpg_u32 height;
};

struct YuvbufSize {
    jpg_u32 width;
    jpg_u32 height;
};

struct YuvbufAddr {
    jpg_uint addrY;
    jpg_uint addrU;
    jpg_uint addrV;
};

struct YuvbufDataEnd {
    jpg_u8 yEndian;
    jpg_u8 uvEndian;
    jpg_u8 reserved0;
    jpg_u8 reserved1;
};

struct YuvbufInfo {
    jpg_u32 channelId;
    jpg_u32 frameId;
    jpg_u32 frameRealId;
    jpg_u32 height;
    jpg_uint sec;
    jpg_uint usec;
    jpg_u32 length;
    jpg_u32 free1;
    jpg_u32 base;
    jpg_u32 fmt;
};

struct YuvbufFrm {
    jpg_u32 fmt;
    jpg_u32 bufSize;
    struct YuvbufRect rect;
    struct YuvbufSize size;
    struct YuvbufAddr addrPhy;
    struct YuvbufAddr addrVir;
    jpg_u32 fd;
    struct YuvbufDataEnd dataEnd;
    jpg_u32 formatPattern;
    void* reserved;
};

struct JpgOpMean {
    jpg_u32 sliceHeight;
    jpg_u32 sliceMode;
    jpg_u32 readyLineNum;
    jpg_u32 sliceNum;
    jpg_u32 isSync;
    jpg_u32 isThumb;
    jpg_u8 mirror;
    jpg_u8 flip;
    jpg_u8 rotation;
    jpg_u32 qualityLevel;
    jpg_uint outParam;
    struct YuvbufFrm tempBuf;
};

struct JpegEncCbParam {
    jpg_uint streamBufPhy;
    jpg_uint streamBufVir;
    jpg_u32 streamSize;
    jpg_u32 sliceHeight;
    jpg_u32 totalHeight;
    jpg_u32 isThumbnail;
};

struct JpegDecCbParam {
    jpg_u32 sliceHeight;
    jpg_u32 totalHeight;
    struct YuvbufFrm* srcImg;
    struct YuvbufDataEnd dataEndian;
};

#define CMR_EVT_JPEG_BASE (1 << 20)
enum JpgJpegEvt {
    CMR_JPEG_ENC_DONE = CMR_EVT_JPEG_BASE,
    CMR_JPEG_DEC_DONE,
    CMR_JPEG_WEXIF_DONE,
    CMR_JPEG_ENC_ERR,
    CMR_JPEG_DEC_ERR,
    CMR_JPEG_ERR,
};

#ifdef __cplusplus
}
#endif

#endif
