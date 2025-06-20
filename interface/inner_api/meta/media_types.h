/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef MEDIA_FOUNDATION_MEDIA_TYPES_H
#define MEDIA_FOUNDATION_MEDIA_TYPES_H

#include <cstdint>  // NOLINT: used it
#include <string_view>
#include <vector>

namespace OHOS {
namespace Media {
namespace Plugins {
/**
 * @enum Media Track Type
 *
 * @since 1.0
 * @version 1.0
 */
enum class MediaType : int32_t {
    UNKNOWN = -1,    ///< Usually treated as DATA
    AUDIO = 0,
    VIDEO = 1,
    SUBTITLE = 2,
    ATTACHMENT,     ///< Opaque data information usually sparse
    DATA,           ///< Opaque data information usually continuous
    TIMEDMETA,
    AUXILIARY = 6,
};

/**
 * @enum Seekable Status.
 *
 * @since 1.0
 * @version 1.0
 */
enum class Seekable : int32_t {
    INVALID = -1,
    UNSEEKABLE = 0,
    SEEKABLE = 1
};

/**
 * @enum Codec Mode.
 *
 * @since 1.0
 * @version 1.0
 */
enum struct CodecMode {
    HARDWARE, ///<  HARDWARE CODEC
    SOFTWARE, ///<  SOFTWARE CODEC
};

/**
 * @enum Mux file output format.
 *
 * @since 1.0
 * @version 1.0
 */
enum class OutputFormat : uint32_t {
    DEFAULT = 0,
    MPEG_4 = 2,
    M4A = 6,
    AMR = 8,
    MP3 = 9,
    WAV = 10,
    AAC = 11,
    FLAC = 12,
};

struct Location {
    float latitude = 0;
    float longitude = 0;
};

/**
 * @enum Enumerates types of Seek Mode.
 *
 * @brief Seek modes, Options that SeekTo() behaviour.
 *
 * @since 1.0
 * @version 1.0
 */
enum struct SeekMode : uint32_t {
    SEEK_NEXT_SYNC = 0,     ///> sync to keyframes after the time point.
    SEEK_PREVIOUS_SYNC,     ///> sync to keyframes before the time point.
    SEEK_CLOSEST_SYNC,      ///> sync to closest keyframes.
    SEEK_CLOSEST,           ///> seek to frames closest the time point.
    SEEK_CLOSEST_INNER,     ///> seek to frames closest the time point for inner accurate seek.
};

/**
 * @enum Buffer flags.
 *
 * @since 1.0
 * @version 1.0
 */
enum class AVBufferFlag : uint32_t {
    NONE = 0,
    /* This signals the end of stream */
    EOS = 1 << 0,
    /* This indicates that the buffer contains the data for a sync frame */
    SYNC_FRAME = 1 << 1,
    /* This indicates that the buffer only contains part of a frame */
    PARTIAL_FRAME = 1 << 2,
    /* This indicated that the buffer contains codec specific data */
    CODEC_DATA = 1 << 3,
    /**
     * Flag is used to discard packets which are required to maintain valid decoder state but are not required
     * for output and should be dropped after decoding.
     */
    DISCARD = 1 << 4,
    /**
     * Flag is used to indicate packets that contain frames that can be discarded by the decoder,
     * I.e. Non-reference frames.
     */
    DISPOSABLE = 1 << 5,
    /**
     * Indicates that the frame is an extended discardable frame. It is not on the main reference path and
     * is referenced only by discardable frames on the branch reference path are discarded by decoder, the
     * frame can be further discarded.
     */
    DISPOSABLE_EXT = 1 << 6,
};

/**
 * @enum Demux file type.
 *
 * @since 1.0
 * @version 1.0
 */
enum class FileType : int32_t {
    UNKNOW = 0,
    MP4 = 101,
    MPEGTS = 102,
    MKV = 103,
    FLV = 104,
    AVI = 105,
    MPEGPS = 106,
    MOV = 107,
    RM = 108,
    AC3 = 109,
    AMR = 201,
    AAC = 202,
    MP3 = 203,
    FLAC = 204,
    OGG = 205,
    M4A = 206,
    WAV = 207,
    APE = 208,
    SRT = 301,
    VTT = 302,
    LRC = 303,
    SAMI = 304,
    ASS = 305
};

/**
 * @enum Media protocol type.
 *
 * @since 1.0
 * @version 1.0
 */
enum struct ProtocolType : uint32_t {
    UNKNOWN, ///< Unknown protocol
    FILE,    ///< File protocol, uri prefix: "file://"
    FD,      ///< File descriptor protocol, uri prefix: "fd://"
    STREAM,  ///< Stream protocol, uri prefix: "stream://"
    HTTP,    ///< Http protocol, uri prefix: "http://"
    HTTPS,   ///< Https protocol, uri prefix: "https://"
    HLS,     ///< Http live streaming protocol, uri prefix: "https://" or "https://" or "file://", suffix: ".m3u8"
    DASH,    ///< Dynamic adaptive streaming over Http protocol, uri prefix: "https://" or "https://", suffix: ".mpd"
    RTSP,    ///< Real time streaming protocol, uri prefix: "rtsp://"
    RTP,     ///< Real-time transport protocol, uri prefix: "rtp://"
    RTMP,    ///< RTMP protocol, uri prefix: "rtmp://"
    FTP,     ///< FTP protocol, uri prefix: "ftp://"
    UDP,     ///< User datagram protocol, uri prefix: "udp://"
};

/**
 * @enum Plugin Type.
 *
 * @since 1.0
 * @version 1.0
 */
enum struct PluginType : int32_t {
    INVALID_TYPE = -1, ///< Invalid plugin
    SOURCE = 1,        ///< reference SourcePlugin
    DEMUXER,           ///< reference DemuxerPlugin
    AUDIO_DECODER,     ///< reference CodecPlugin
    AUDIO_ENCODER,     ///< reference CodecPlugin
    VIDEO_DECODER,     ///< reference CodecPlugin
    VIDEO_ENCODER,     ///< reference CodecPlugin
    AUDIO_SINK,        ///< reference AudioSinkPlugin
    VIDEO_SINK,        ///< reference VideoSinkPlugin
    MUXER,             ///< reference MuxerPlugin
    OUTPUT_SINK,       ///< reference OutputSinkPlugin
    GENERIC_PLUGIN,    ///< generic plugin can be used to represent any user extended plugin
};

/**
 * @enum Plugin running state.
 *
 * @since 1.0
 * @version 1.0
 */
enum struct State : int32_t {
    CREATED = 0,     ///< Indicates the status of the plugin when it is constructed.
    ///< The plug-in will not be restored in the entire life cycle.
    INITIALIZED = 1, ///< Plugin global resource initialization completion status.
    PREPARED = 2,    ///< Status of parameters required for plugin running.
    RUNNING = 3,     ///< The system enters the running state after call start().
    PAUSED = 4,      ///< Plugin temporarily stops processing data. This state is optional.
    DESTROYED = -1,  ///< Plugin destruction state. In this state, all resources are released.
    INVALID = -2,    ///< An error occurs in any state and the plugin enters the invalid state.
};

/**
 * @enum codec name.
 *
 * @since 4.1
 */
class CodecName {
public:
    static constexpr std::string_view AUDIO_DECODER_MP3_NAME = "OH.Media.Codec.Decoder.Audio.Mpeg";
    static constexpr std::string_view AUDIO_DECODER_AAC_NAME = "OH.Media.Codec.Decoder.Audio.AAC";
    static constexpr std::string_view AUDIO_DECODER_API9_AAC_NAME = "avdec_aac";
    static constexpr std::string_view AUDIO_DECODER_VORBIS_NAME = "OH.Media.Codec.Decoder.Audio.Vorbis";
    static constexpr std::string_view AUDIO_DECODER_FLAC_NAME = "OH.Media.Codec.Decoder.Audio.Flac";
    static constexpr std::string_view AUDIO_DECODER_AMRNB_NAME = "OH.Media.Codec.Decoder.Audio.Amrnb";
    static constexpr std::string_view AUDIO_DECODER_AMRWB_NAME = "OH.Media.Codec.Decoder.Audio.Amrwb";

    static constexpr std::string_view AUDIO_ENCODER_FLAC_NAME = "OH.Media.Codec.Encoder.Audio.Flac";
    static constexpr std::string_view AUDIO_ENCODER_AAC_NAME = "OH.Media.Codec.Encoder.Audio.AAC";
    static constexpr std::string_view AUDIO_ENCODER_API9_AAC_NAME = "avenc_aac";

    static constexpr std::string_view VIDEO_DECODER_AVC_NAME = "OH.Media.Codec.Decoder.Video.AVC";
    static constexpr std::string_view VIDEO_DECODER_MPEG2_NAME = "OH.Media.Codec.Decoder.Video.MPEG2";
    static constexpr std::string_view VIDEO_DECODER_MPEG4_NAME = "OH.Media.Codec.Decoder.Video.MPEG4";
    static constexpr std::string_view VIDEO_DECODER_H263_NAME = "OH.Media.Codec.Decoder.Video.H263";
private:
    CodecName() = delete;
    ~CodecName() = delete;
};

/**
 * The tag content is stored in key-value format.
 */
using CodecConfig = std::vector<uint8_t>;

/**
 * DRM structure definition
 */
#define META_DRM_KEY_ID_SIZE                    16
#define META_DRM_IV_SIZE                        16
#define META_DRM_MAX_SUB_SAMPLE_NUM             64
#define META_DRM_MAX_DRM_UUID_LEN               16
#define META_DRM_MAX_DRM_PSSH_LEN               2048

enum struct MetaDrmCencAlgorithm : uint32_t {
    META_DRM_ALG_CENC_UNENCRYPTED = 0x0,
    META_DRM_ALG_CENC_AES_CTR = 0x1,
    META_DRM_ALG_CENC_AES_WV = 0x2,
    META_DRM_ALG_CENC_AES_CBC = 0x3,
    META_DRM_ALG_CENC_SM4_CBC = 0x4,
    META_DRM_ALG_CENC_SM4_CTR,
};

enum struct MetaDrmCencInfoMode : uint32_t {
    /* key/iv/subsample set. */
    META_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_SET = 0x0,
    /* key/iv/subsample not set. */
    META_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_NOT_SET = 0x1,
};

struct _MetaDrmSubSample {
    uint32_t clearHeaderLen;
    uint32_t payLoadLen;
};
typedef struct _MetaDrmSubSample MetaDrmSubSample;

struct _MetaDrmCencInfo {
    MetaDrmCencAlgorithm algo;
    uint8_t keyId[META_DRM_KEY_ID_SIZE];
    uint32_t keyIdLen;
    uint8_t iv[META_DRM_IV_SIZE];
    uint32_t ivLen;
    uint32_t encryptBlocks;
    uint32_t skipBlocks;
    uint32_t firstEncryptOffset;
    MetaDrmSubSample subSamples[META_DRM_MAX_SUB_SAMPLE_NUM];
    uint32_t subSampleNum;
    MetaDrmCencInfoMode mode;
};
typedef struct _MetaDrmCencInfo MetaDrmCencInfo;

struct _MetaDrmInfo {
    uint32_t uuidLen;
    uint8_t uuid[META_DRM_MAX_DRM_UUID_LEN];
    uint32_t psshLen;
    uint8_t pssh[META_DRM_MAX_DRM_PSSH_LEN];
};
typedef struct _MetaDrmInfo MetaDrmInfo;
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MEDIA_FOUNDATION_MEDIA_TYPES_H
