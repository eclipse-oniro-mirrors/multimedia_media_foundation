/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_PLUGIN_LIST_H
#define HISTREAMER_PLUGIN_LIST_H

#include <vector>
#include <string>
#include <cstring>
#include "media_types.h"

namespace OHOS {
namespace Media {
namespace Plugins {
struct PluginDescription {
    std::string pluginName;
    std::string packageName;
    PluginType pluginType;
    std::string cap;
    int32_t rank;
};

class PluginList {
public:
    ~PluginList();
    static PluginList &GetInstance();
    std::vector<PluginDescription> GetAllPlugins();
    std::vector<PluginDescription> GetPluginsByCap(PluginType pluginType, std::string needCap);
    PluginDescription GetPluginByName(std::string name);
    std::vector<PluginDescription> GetPluginsByType(PluginType pluginType);
private:
    PluginList();

    void AddDataSourceStreamPlugins();
    void AddFileFdSourcePlugins();
    void AddFileSourcePlugins();
    void AddHttpSourcePlugins();
    void AddAacDemuxerPlugin();
    void AddAmrDemuxerPlugin();
    void AddAmrnbDemuxerPlugin();
    void AddAmrwbDemuxerPlugin();
    void AddApeDemuxerPlugin();
    void AddAsfDemuxerPlugin();
    void AddAsfoDemuxerPlugin();
    void AddFlacDemuxerPlugin();
    void AddFlvDemuxerPlugin();
    void AddMatroskaDemuxerPlugin();
    void AddMovDemuxerPlugin();
    void AddMp3DemuxerPlugin();
    void AddMpegDemuxerPlugin();
    void AddMpegtsDemuxerPlugin();
    void AddAviDemuxerPlugin();
    void AddSrtDemuxerPlugin();
    void AddWebvttDemuxerPlugin();
    void AddOggDemuxerPlugin();
    void AddWavDemuxerPlugin();
    void AddFFmpegDemuxerPlugins();
    void AddMpegAudioDecoderPlugin();
    void AddAacAudioDecoderPlugin();
    void AddFFmpegAudioDecodersPlugins();
    void AddAudioVividDecodersPlugins();
    void AddL2hcEncodersPlugins();
    void AddL2hcDecodersPlugins();
    void AddG711muAudioDecoderPlugins();
    void AddG711aAudioDecoderPlugins();
    void AddLbvcAudioDecoderPlugins();
    void AddOpusAudioDecoderPlugins();
    void AddRawAudioDecoderPlugins();
    void AddAudioServerSinkPlugins();
    void AddFFmpegAudioEncodersPlugins();
    void AddG711muAudioEncoderPlugins();
    void AddLbvcAudioEncoderPlugins();
    void AddOpusAudioEncoderPlugins();
    void AddAmrwbAudioEncoderPlugins();
    void AddAmrnbAudioEncoderPlugins();
    void AddMp3AudioEncoderPlugins();
    void AddFFmpegMuxerPlugins();
    void AddFFmpegFlacMuxerplugins();
    void AddAudioVendorAacEncodersPlugin();
#ifdef SUPPORT_CODEC_RM
    void AddRmDemuxerPlugin();
#endif
#ifdef SUPPORT_CODEC_COOK
    void AddCookAudioDecoderPlugins();
#endif
    void AddAc3DemuxerPlugin();
    void AddAc3AudioDecoderPlugins();
#ifdef SUPPORT_DEMUXER_LRC
    void AddLrcDemuxerPlugin();
#endif
#ifdef SUPPORT_DEMUXER_SAMI
    void AddSamiDemuxerPlugin();
#endif
#ifdef SUPPORT_DEMUXER_ASS
    void AddAssDemuxerPlugin();
#endif

    std::vector<PluginDescription> pluginDescriptionList_;
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_PLUGIN_LIST_H
