/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd.
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

#ifndef MEDIA_FOUNDATION_VIDEO_TYPES_H
#define MEDIA_FOUNDATION_VIDEO_TYPES_H

#include <cstdint>  // NOLINT: used it

namespace OHOS {
namespace Media {
namespace Plugin {
enum class VideoScaleType {
    VIDEO_SCALE_TYPE_FIT,
    VIDEO_SCALE_TYPE_FIT_CROP,
};

/**
 * @enum Video Pixel Format.
 *
 * @since 1.0
 * @version 1.0
 */
enum struct VideoPixelFormat : int32_t {
    UNKNOWN = -1,
    YUV420P = 0,        ///< planar YUV 4:2:0, 1 Cr & Cb sample per 2x2 Y samples
    YUVI420 = 1,        ///< planar YUV 4:2:0, 1 Cr & Cb sample per 2x2 Y samples
    NV12 = 2,           ///< semi-planar YUV 4:2:0, UVUV...
    NV21 = 3,           ///< semi-planar YUV 4:2:0, VUVU...
    SURFACE_FORMAT = 4, ///< format from surface
    RGBA = 5,           ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
    YUV410P,            ///< planar YUV 4:1:0, 1 Cr & Cb sample per 4x4 Y samples
    YUV411P,            ///< planar YUV 4:1:1, 1 Cr & Cb sample per 4x1 Y samples
    YUYV422,            ///< packed YUV 4:2:2, Y0 Cb Y1 Cr
    YUV422P,            ///< planar YUV 4:2:2, 1 Cr & Cb sample per 2x1 Y samples
    YUV444P,            ///< planar YUV 4:4:4, 1 Cr & Cb sample per 1x1 Y samples
    ARGB,               ///< packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
    ABGR,               ///< packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
    BGRA,               ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
    RGB24,              ///< packed RGB 8:8:8, RGBRGB...
    BGR24,              ///< packed RGB 8:8:8, BGRBGR...
    PAL8,               ///< 8 bit with AV_PIX_FMT_RGB32 palette
    GRAY8,              ///< Y
    MONOWHITE,          ///< Y, 0 is white, 1 is black, in each byte pixels are ordered from the msb to the lsb
    MONOBLACK,          ///< Y, 0 is black, 1 is white, in each byte pixels are ordered from the msb to the lsb
    YUVJ420P,           ///< planar YUV 4:2:0, 12bpp, full scale (JPEG)
    YUVJ422P,           ///< planar YUV 4:2:2, 16bpp, full scale (JPEG)
    YUVJ444P,           ///< planar YUV 4:4:4, 24bpp, full scale (JPEG)
};

/**
 * @enum Video H264/AVC profile.
 *
 * @since 1.0
 * @version 1.0
 */
enum struct VideoH264Profile : int32_t {
    UNKNOWN,
    BASELINE,  ///< Baseline profile
    MAIN,      ///< Main profile
    EXTENDED,  ///< Extended profile
    HIGH,      ///< High profile
    HIGH10,    ///< High 10 profile
    HIGH422,   ///< High 4:2:2 profile
    HIGH444,   ///< High 4:4:4 profile
};

/**
 * @enum Video Bit Stream format.
 *
 * @since 1.0
 * @version 1.0
 */
enum struct VideoBitStreamFormat : uint32_t {
    UNKNOWN,
    AVC1,  // H264 bit stream format
    HEVC,  // H265 bit stream format
    ANNEXB, // H264, H265 bit stream format
};

enum VideoEncodeBitrateMode : int32_t {
    CBR = 0, // constant bit rate mode.
    VBR = 1, // variable bit rate mode.
    CQ = 2, // constant quality mode.
};

enum class ColorPrimary : int32_t {
    BT709 = 1,
    UNSPECIFIED = 2,
    BT470_M = 4,
    BT601_625 = 5,
    BT601_525 = 6,
    SMPTE_ST240 = 7,
    GENERIC_FILM = 8,
    BT2020 = 9,
    SMPTE_ST428 = 10,
    P3DCI = 11,
    P3D65 = 12,
};

enum class TransferCharacteristic : int32_t {
    BT709 = 1,
    UNSPECIFIED = 2,
    GAMMA_2_2 = 4,
    GAMMA_2_8 = 5,
    BT601 = 6,
    SMPTE_ST240 = 7,
    LINEAR = 8,
    LOG = 9,
    LOG_SQRT = 10,
    IEC_61966_2_4 = 11,
    BT1361 = 12,
    IEC_61966_2_1 = 13,
    BT2020_10BIT = 14,
    BT2020_12BIT = 15,
    PQ = 16,
    SMPTE_ST428 = 17,
    HLG = 18,
};

enum class MatrixCoefficient : int32_t {
    IDENTITY = 0,
    BT709 = 1,
    UNSPECIFIED = 2,
    FCC = 4,
    BT601_625 = 5,
    BT601_525 = 6,
    SMPTE_ST240 = 7,
    YCGCO = 8,
    BT2020_NCL = 9,
    BT2020_CL = 10,
    SMPTE_ST2085 = 11,
    CHROMATICITY_NCL = 12,
    CHROMATICITY_CL = 13,
    ICTCP = 14,
};

enum class ChromaLocation {
    UNSPECIFIED = 0,
    LEFT = 1, ///< MPEG-2/4 4:2:0, H.264 default for 4:2:0
    CENTER = 2, ///< MPEG-1 4:2:0, JPEG 4:2:0, H.263 4:2:0
    TOPLEFT = 3, ///< ITU-R 601, SMPTE 274M 296M S314M(DV 4:1:1), mpeg2 4:2:2
    TOP = 4,
    BOTTOMLEFT = 5,
    BOTTOM = 6,
};

enum VideoRotation : int32_t {
    VIDEO_ROTATION_0 = 0,
    VIDEO_ROTATION_90 = 90,
    VIDEO_ROTATION_180 = 180,
    VIDEO_ROTATION_270 = 270,
};

enum AVCProfile : int32_t {
    AVC_PROFILE_BASELINE = 0,
    AVC_PROFILE_CONSTRAINED_BASELINE = 1,
    AVC_PROFILE_CONSTRAINED_HIGH = 2,
    AVC_PROFILE_EXTENDED = 3,
    AVC_PROFILE_HIGH = 4,
    AVC_PROFILE_HIGH_10 = 5,
    AVC_PROFILE_HIGH_422 = 6,
    AVC_PROFILE_HIGH_444 = 7,
    AVC_PROFILE_MAIN = 8,
};

enum HEVCProfile : int32_t {
    HEVC_PROFILE_MAIN = 0,
    HEVC_PROFILE_MAIN_10 = 1,
    HEVC_PROFILE_MAIN_STILL = 2,
    HEVC_PROFILE_MAIN_10_HDR10 = 3,
    HEVC_PROFILE_MAIN_10_HDR10_PLUS = 4,
    HEVC_PROFILE_UNKNOW = -1,
};

enum MPEG2Profile : int32_t {
    MPEG2_PROFILE_422 = 0,
    MPEG2_PROFILE_HIGH = 1,
    MPEG2_PROFILE_MAIN = 2,
    MPEG2_PROFILE_SNR = 3,
    MPEG2_PROFILE_SIMPLE = 4,
    MPEG2_PROFILE_SPATIAL = 5,
};

enum MPEG4Profile : int32_t {
    MPEG4_PROFILE_ADVANCED_CODING = 0,
    MPEG4_PROFILE_ADVANCED_CORE = 1,
    MPEG4_PROFILE_ADVANCED_REAL_TIME = 2,
    MPEG4_PROFILE_ADVANCED_SCALABLE = 3,
    MPEG4_PROFILE_ADVANCED_SIMPLE = 4,
    MPEG4_PROFILE_BASIC_ANIMATED = 5,
    MPEG4_PROFILE_CORE = 6,
    MPEG4_PROFILE_CORE_SCALABLE = 7,
    MPEG4_PROFILE_HYBRID = 8,
    MPEG4_PROFILE_MAIN = 9,
    MPEG4_PROFILE_NBIT = 10,
    MPEG4_PROFILE_SCALABLE_TEXTURE = 11,
    MPEG4_PROFILE_SIMPLE = 12,
    MPEG4_PROFILE_SIMPLE_FBA = 13,
    MPEG4_PROFILE_SIMPLE_FACE = 14,
    MPEG4_PROFILE_SIMPLE_SCALABLE = 15,
};

enum H263Profile : int32_t {
    H263_PROFILE_BACKWARD_COMPATIBLE = 0,
    H263_PROFILE_BASELINE = 1,
    H263_PROFILE_H320_CODING = 2,
    H263_PROFILE_HIGH_COMPRESSION = 3,
    H263_PROFILE_HIGH_LATENCY = 4,
    H263_PROFILE_ISW_V2 = 5,
    H263_PROFILE_ISW_V3 = 6,
    H263_PROFILE_INTERLACE = 7,
    H263_PROFILE_INTERNET = 8,
};

enum VP8Profile : int32_t {
    VP8_PROFILE_MAIN = 0,
};

enum AVCLevel : int32_t {
    AVC_LEVEL_1 = 0,
    AVC_LEVEL_1b = 1,
    AVC_LEVEL_11 = 2,
    AVC_LEVEL_12 = 3,
    AVC_LEVEL_13 = 4,
    AVC_LEVEL_2 = 5,
    AVC_LEVEL_21 = 6,
    AVC_LEVEL_22 = 7,
    AVC_LEVEL_3 = 8,
    AVC_LEVEL_31 = 9,
    AVC_LEVEL_32 = 10,
    AVC_LEVEL_4 = 11,
    AVC_LEVEL_41 = 12,
    AVC_LEVEL_42 = 13,
    AVC_LEVEL_5 = 14,
    AVC_LEVEL_51 = 15,
};

enum HEVCLevel : int32_t {
    HEVC_LEVEL_1 = 0,
    HEVC_LEVEL_2 = 1,
    HEVC_LEVEL_21 = 2,
    HEVC_LEVEL_3 = 3,
    HEVC_LEVEL_31 = 4,
    HEVC_LEVEL_4 = 5,
    HEVC_LEVEL_41 = 6,
    HEVC_LEVEL_5 = 7,
    HEVC_LEVEL_51 = 8,
    HEVC_LEVEL_52 = 9,
    HEVC_LEVEL_6 = 10,
    HEVC_LEVEL_61 = 11,
    HEVC_LEVEL_62 = 12,
    HEVC_LEVEL_UNKNOW = -1,
};

enum MPEG2Level : int32_t {
    MPEG2_LEVEL_LL = 0,
    MPEG2_LEVEL_ML = 1,
    MPEG2_LEVEL_H14 = 2,
    MPEG2_LEVEL_HL = 3,
};

enum MPEG4Level : int32_t {
    MPEG4_LEVEL_0 = 0,
    MPEG4_LEVEL_0B = 1,
    MPEG4_LEVEL_1 = 2,
    MPEG4_LEVEL_2 = 3,
    MPEG4_LEVEL_3 = 4,
    MPEG4_LEVEL_4 = 5,
    MPEG4_LEVEL_4A = 6,
    MPEG4_LEVEL_5 = 7,
};
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // MEDIA_FOUNDATION_VIDEO_TYPES_H
