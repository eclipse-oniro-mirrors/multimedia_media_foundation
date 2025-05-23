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

#define HST_LOG_TAG "DemuxerFilter"

#include "pipeline/filters/demux/demuxer_filter.h"
#include <algorithm>
#include "foundation/cpp_ext/type_traits_ext.h"
#include "foundation/log.h"
#include "foundation/utils/constants.h"
#include "foundation/utils/dump_buffer.h"
#include "foundation/utils/steady_clock.h"
#include "pipeline/core/compatible_check.h"
#include "pipeline/factory/filter_factory.h"
#include "pipeline/filters/common/plugin_utils.h"
#include "plugin/common/plugin_time.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<DemuxerFilter> g_registerFilterHelper("builtin.player.demuxer");

class DemuxerFilter::DataSourceImpl : public Plugin::DataSourceHelper {
public:
    explicit DataSourceImpl(const DemuxerFilter& filter);
    ~DataSourceImpl() override = default;
    Plugin::Status ReadAt(int64_t offset, std::shared_ptr<Plugin::Buffer>& buffer, size_t expectedLen) override;
    Plugin::Status GetSize(uint64_t& size) override;
    Plugin::Seekable GetSeekable() override;

private:
    const DemuxerFilter& filter;
};

DemuxerFilter::DataSourceImpl::DataSourceImpl(const DemuxerFilter& filter) : filter(filter)
{
}

/**
 * ReadAt Plugin::DataSource::ReadAt implementation.
 * @param offset offset in media stream.
 * @param buffer caller allocate real buffer.
 * @param expectedLen buffer size wanted to read.
 * @return read result.
 */
Plugin::Status DemuxerFilter::DataSourceImpl::ReadAt(int64_t offset, std::shared_ptr<Plugin::Buffer>& buffer,
                                                     size_t expectedLen)
{
    if (!buffer || buffer->IsEmpty() || expectedLen == 0 || !filter.IsOffsetValid(offset)) {
        MEDIA_LOG_E("ReadAt failed, buffer empty: " PUBLIC_LOG_D32 ", expectedLen: " PUBLIC_LOG_D32
                    ", offset: " PUBLIC_LOG_D64, !buffer, static_cast<int>(expectedLen), offset);
        return Plugin::Status::ERROR_UNKNOWN;
    }
    Plugin::Status rtv = Plugin::Status::OK;
    switch (filter.pluginState_.load()) {
        case DemuxerState::DEMUXER_STATE_NULL:
            rtv = Plugin::Status::ERROR_WRONG_STATE;
            MEDIA_LOG_E("ReadAt error due to DEMUXER_STATE_NULL");
            break;
        case DemuxerState::DEMUXER_STATE_PARSE_HEADER: {
            if (filter.getRange_(static_cast<uint64_t>(offset), expectedLen, buffer)) {
                DUMP_BUFFER2FILE(DEMUXER_INPUT_PEEK, buffer);
            } else {
                rtv = Plugin::Status::ERROR_NOT_ENOUGH_DATA;
            }
            break;
        }
        case DemuxerState::DEMUXER_STATE_PARSE_FRAME: {
            if (filter.getRange_(static_cast<uint64_t>(offset), expectedLen, buffer)) {
                DUMP_BUFFER2LOG("Demuxer GetRange", buffer, offset);
                DUMP_BUFFER2FILE(DEMUXER_INPUT_GET, buffer);
            } else {
                rtv = Plugin::Status::END_OF_STREAM;
            }
            break;
        }
        default:
            break;
    }
    return rtv;
}

Plugin::Status DemuxerFilter::DataSourceImpl::GetSize(uint64_t& size)
{
    size = filter.mediaDataSize_;
    return (filter.mediaDataSize_ > 0) ? Plugin::Status::OK : Plugin::Status::ERROR_WRONG_STATE;
}

Plugin::Seekable DemuxerFilter::DataSourceImpl::GetSeekable()
{
    return filter.seekable_;
}

DemuxerFilter::DemuxerFilter(std::string name)
    : FilterBase(std::move(name)),
      seekable_(Plugin::Seekable::INVALID),
      uri_(),
      mediaDataSize_(0),
      task_(nullptr),
      typeFinder_(nullptr),
      dataPacker_(nullptr),
      pluginName_(),
      plugin_(nullptr),
      pluginState_(DemuxerState::DEMUXER_STATE_NULL),
      pluginAllocator_(nullptr),
      dataSource_(std::make_shared<DataSourceImpl>(*this)),
      mediaMetaData_()
{
    filterType_ = FilterType::DEMUXER;
    dataPacker_ = std::make_shared<DataPacker>();
    task_ = std::make_shared<OSAL::Task>("DemuxerFilter");
    MEDIA_LOG_D("ctor called");
}

DemuxerFilter::~DemuxerFilter()
{
    MEDIA_LOG_I("dtor called");
    StopTask(true);
    if (plugin_) {
        plugin_->Deinit();
    }
}

void DemuxerFilter::Init(EventReceiver* receiver, FilterCallback* callback)
{
    this->eventReceiver_ = receiver;
    this->callback_ = callback;
    inPorts_.clear();
    outPorts_.clear();
    inPorts_.push_back(std::make_shared<Pipeline::InPort>(this, PORT_NAME_DEFAULT));
    state_ = FilterState::INITIALIZED;
}

ErrorCode DemuxerFilter::Start()
{
    MEDIA_LOG_I("Start called.");
    if (task_) {
        task_->Start();
    }
    return FilterBase::Start();
}

ErrorCode DemuxerFilter::Stop()
{
    MEDIA_LOG_I("Stop called.");
    dataPacker_->Stop();
    StopTask(true);
    Reset();
    if (!outPorts_.empty()) {
        PortInfo portInfo;
        portInfo.type = PortType::OUT;
        portInfo.ports.reserve(outPorts_.size());
        for (const auto& outPort : outPorts_) {
            portInfo.ports.push_back({outPort->GetName(), false});
        }
        if (callback_) {
            callback_->OnCallback(FilterCallbackType::PORT_REMOVE, static_cast<Filter*>(this), portInfo);
        }
    }
    return FilterBase::Stop();
}

ErrorCode DemuxerFilter::Pause()
{
    MEDIA_LOG_I("Pause called");
    return FilterBase::Pause();
}

void DemuxerFilter::FlushStart()
{
    MEDIA_LOG_I("FlushStart entered");
    task_->Pause();
    dataPacker_->Flush();
}

void DemuxerFilter::FlushEnd()
{
    MEDIA_LOG_I("FlushEnd entered");
    if (task_) {
        task_->Start();
    }
}

ErrorCode DemuxerFilter::SetParameter(int32_t key, const Plugin::Any& value)
{
    FALSE_RETURN_V_MSG(plugin_ != nullptr, ErrorCode::ERROR_INVALID_OPERATION, "plugin is nullptr");
    return TranslatePluginStatus(plugin_->SetParameter(static_cast<Plugin::Tag>(key), value));
}

ErrorCode DemuxerFilter::GetParameter(int32_t key, Plugin::Any& value)
{
    FALSE_RETURN_V_MSG(plugin_ != nullptr, ErrorCode::ERROR_INVALID_OPERATION, "plugin is nullptr");
    return TranslatePluginStatus(plugin_->SetParameter(static_cast<Plugin::Tag>(key), value));
}

ErrorCode DemuxerFilter::Prepare()
{
    MEDIA_LOG_I("Prepare called");
    dataPacker_->Start();
    pluginState_ = DemuxerState::DEMUXER_STATE_NULL;
    task_->RegisterHandler([this] { DemuxerLoop(); });
    Pipeline::WorkMode mode;
    GetInPort(PORT_NAME_DEFAULT)->Activate({Pipeline::WorkMode::PULL, Pipeline::WorkMode::PUSH}, mode);
    if (mode == Pipeline::WorkMode::PULL) {
        dataPacker_->Flush();
        ActivatePullMode();
    } else {
        ActivatePushMode();
    }
    state_ = FilterState::PREPARING;
    MEDIA_LOG_I("demuexfilter Prepare success");
    return ErrorCode::SUCCESS;
}

ErrorCode DemuxerFilter::PushData(const std::string& inPort, const AVBufferPtr& buffer, int64_t offset)
{
    MEDIA_LOG_D("PushData for port: " PUBLIC_LOG_S, inPort.c_str());
    if (buffer->flag & BUFFER_FLAG_EOS) {
        dataPacker_->SetEos();
    } else {
        dataPacker_->PushData(std::move(buffer), offset);
    }
    return ErrorCode::SUCCESS;
}

bool DemuxerFilter::Negotiate(const std::string& inPort,
                              const std::shared_ptr<const Plugin::Capability>& upstreamCap,
                              Plugin::Capability& negotiatedCap,
                              const Plugin::Meta& upstreamParams,
                              Plugin::Meta& downstreamParams)
{
    (void)inPort;
    (void)upstreamCap;
    (void)negotiatedCap;
    (void)upstreamParams;
    (void)downstreamParams;
    return true;
}

bool DemuxerFilter::Configure(const std::string& inPort, const std::shared_ptr<const Plugin::Meta>& upstreamMeta,
                              Plugin::Meta& upstreamParams, Plugin::Meta& downstreamParams)
{
    (void)downstreamParams;
    FALSE_LOG_MSG(upstreamMeta->Get<Plugin::Tag::MEDIA_FILE_SIZE>(mediaDataSize_), "Get media file size  failed.");
    FALSE_LOG_MSG(upstreamMeta->Get<Plugin::Tag::MEDIA_SEEKABLE>(seekable_), "Get MEDIA_SEEKABLE failed");
    FALSE_LOG_MSG(upstreamMeta->Get<Plugin::Tag::MEDIA_FILE_URI>(uri_), "Get MEDIA_FILE_URI failed");
    return true;
}

ErrorCode DemuxerFilter::SeekTo(int64_t seekTime, Plugin::SeekMode mode, int64_t& realSeekTime)
{
    if (!plugin_) {
        MEDIA_LOG_E("SeekTo failed due to no valid plugin");
        return ErrorCode::ERROR_INVALID_OPERATION;
    }
    auto rtv = TranslatePluginStatus(plugin_->SeekTo(-1, seekTime, mode, realSeekTime));
    if (rtv != ErrorCode::SUCCESS) {
        MEDIA_LOG_E("SeekTo failed with return value: " PUBLIC_LOG_D32, static_cast<int>(rtv));
    }
    return rtv;
}

std::vector<std::shared_ptr<Plugin::Meta>> DemuxerFilter::GetStreamMetaInfo() const
{
    return mediaMetaData_.trackMetas;
}

std::shared_ptr<Plugin::Meta> DemuxerFilter::GetGlobalMetaInfo() const
{
    return mediaMetaData_.globalMeta;
}

void DemuxerFilter::StopTask(bool force)
{
    if (force || pluginState_.load() != DemuxerState::DEMUXER_STATE_PARSE_FRAME) {
        if (task_) {
            task_->Stop();
        }
    }
}

void DemuxerFilter::Reset()
{
    mediaMetaData_.globalMeta.reset();
    mediaMetaData_.trackMetas.clear();
    mediaMetaData_.trackInfos.clear();
}

void DemuxerFilter::InitTypeFinder()
{
    if (!typeFinder_) {
        typeFinder_ = std::make_shared<TypeFinder>();
    }
}

bool DemuxerFilter::CreatePlugin(std::string pluginName)
{
    if (plugin_) {
        plugin_->Deinit();
    }
    plugin_ = Plugin::PluginManager::Instance().CreateDemuxerPlugin(pluginName);
    if (!plugin_ || plugin_->Init() != Plugin::Status::OK) {
        MEDIA_LOG_E("CreatePlugin " PUBLIC_LOG_S " failed.", pluginName.c_str());
        return false;
    }
    plugin_->SetCallback(this);
    pluginAllocator_ = plugin_->GetAllocator();
    pluginName_.swap(pluginName);
    return true;
}

bool DemuxerFilter::InitPlugin(std::string pluginName)
{
    if (pluginName.empty()) {
        return false;
    }
    if (pluginName_ != pluginName) {
        FALSE_RETURN_V(CreatePlugin(std::move(pluginName)), false);
    } else {
        if (plugin_->Reset() != Plugin::Status::OK) {
            FALSE_RETURN_V(CreatePlugin(std::move(pluginName)), false);
        }
    }
    MEDIA_LOG_I("InitPlugin, " PUBLIC_LOG_S " used.", pluginName_.c_str());
    (void)plugin_->SetDataSource(std::reinterpret_pointer_cast<Plugin::DataSourceHelper>(dataSource_));
    pluginState_ = DemuxerState::DEMUXER_STATE_PARSE_HEADER;
    return plugin_->Prepare() == Plugin::Status::OK;
}

void DemuxerFilter::ActivatePullMode()
{
    MEDIA_LOG_D("ActivatePullMode called");
    InitTypeFinder();
    checkRange_ = [this](uint64_t offset, uint32_t size) {
        uint64_t curOffset = offset;
        if (dataPacker_->IsDataAvailable(offset, size, curOffset)) {
            return true;
        }
        MEDIA_LOG_DD("IsDataAvailable false, require offset " PUBLIC_LOG_D64 ", DataPacker data offset end - curOffset "
                     PUBLIC_LOG_D64, offset, curOffset);
        AVBufferPtr bufferPtr = std::make_shared<AVBuffer>();
        bufferPtr->AllocMemory(pluginAllocator_, size);
        auto ret = inPorts_.front()->PullData(curOffset, size, bufferPtr);
        if (ret == ErrorCode::SUCCESS) {
            dataPacker_->PushData(std::move(bufferPtr), curOffset);
            return true;
        } else if (ret == ErrorCode::END_OF_STREAM) {
            // hasDataToRead is ture if there is some data in data packer can be read.
            bool hasDataToRead = offset < curOffset && (!dataPacker_->IsEmpty());
            if (hasDataToRead) {
                dataPacker_->SetEos();
            } else {
                dataPacker_->Flush();
            }
            return hasDataToRead;
        } else {
            MEDIA_LOG_E("PullData from source filter failed " PUBLIC_LOG_D32, ret);
        }
        return false;
    };
    peekRange_ = [this](uint64_t offset, size_t size, AVBufferPtr& bufferPtr) -> bool {
        if (checkRange_(offset, size)) {
            return dataPacker_->PeekRange(offset, size, bufferPtr);
        }
        return false;
    };
    getRange_ = [this](uint64_t offset, size_t size, AVBufferPtr& bufferPtr) -> bool {
        if (checkRange_(offset, size)) {
            auto ret = dataPacker_->GetRange(offset, size, bufferPtr);
            return ret;
        }
        return false;
    };
    typeFinder_->Init(uri_, mediaDataSize_, checkRange_, peekRange_);
    std::string type = typeFinder_->FindMediaType();
    MEDIA_LOG_I("FindMediaType result : type : " PUBLIC_LOG_S ", uri_ : " PUBLIC_LOG_S ", mediaDataSize_ : "
        PUBLIC_LOG_U64, type.c_str(), uri_.c_str(), mediaDataSize_);
    MediaTypeFound(std::move(type));
}

void DemuxerFilter::ActivatePushMode()
{
    MEDIA_LOG_D("ActivatePushMode called");
    InitTypeFinder();
    checkRange_ = [this](uint64_t offset, uint32_t size) {
        return !dataPacker_->IsEmpty(); // True if there is some data
    };
    peekRange_ = [this](uint64_t offset, size_t size, AVBufferPtr& bufferPtr) -> bool {
        return dataPacker_->PeekRange(offset, size, bufferPtr);
    };
    getRange_ = [this](uint64_t offset, size_t size, AVBufferPtr& bufferPtr) -> bool {
        // In push mode, ignore offset, always get data from the start of the data packer.
        return dataPacker_->GetRange(size, bufferPtr);
    };
    typeFinder_->Init(uri_, mediaDataSize_, checkRange_, peekRange_);
    typeFinder_->FindMediaTypeAsync([this](std::string pluginName) { MediaTypeFound(std::move(pluginName)); });
}

void DemuxerFilter::MediaTypeFound(std::string pluginName)
{
    if (InitPlugin(std::move(pluginName))) {
        task_->Start();
    } else {
        MEDIA_LOG_E("MediaTypeFound init plugin error.");
        OnEvent({name_, EventType::EVENT_ERROR, ErrorCode::ERROR_UNSUPPORTED_FORMAT});
    }
}

void DemuxerFilter::InitMediaMetaData(const Plugin::MediaInfoHelper& mediaInfo)
{
    mediaMetaData_.globalMeta = std::make_shared<Plugin::Meta>(mediaInfo.globalMeta);
    mediaMetaData_.trackMetas.clear();
    int trackCnt = 0;
    for (auto& trackMeta : mediaInfo.trackMeta) {
        mediaMetaData_.trackMetas.push_back(std::make_shared<Plugin::Meta>(trackMeta));
        if (!trackMeta.Empty()) {
            ++trackCnt;
        }
    }
    mediaMetaData_.trackInfos.reserve(trackCnt);
}

bool DemuxerFilter::IsOffsetValid(int64_t offset) const
{
    if (seekable_ == Plugin::Seekable::SEEKABLE) {
        return mediaDataSize_ == 0 || offset <= static_cast<int64_t>(mediaDataSize_);
    }
    return true;
}

bool DemuxerFilter::PrepareStreams(const Plugin::MediaInfoHelper& mediaInfo)
{
    MEDIA_LOG_I("PrepareStreams called");
    InitMediaMetaData(mediaInfo);
    outPorts_.clear();
    int streamCnt = mediaInfo.trackMeta.size();
    PortInfo portInfo;
    portInfo.type = PortType::OUT;
    portInfo.ports.reserve(streamCnt);
    int audioTrackCnt = 0;
    for (int i = 0; i < streamCnt; ++i) {
        if (mediaInfo.trackMeta[i].Empty()) {
            MEDIA_LOG_E("PrepareStreams, unsupported stream with trackId = " PUBLIC_LOG_D32, i);
            continue;
        }
        std::string mime;
        uint32_t trackId = 0;
        if (!mediaInfo.trackMeta[i].Get<Plugin::Tag::MIME>(mime) ||
            !mediaInfo.trackMeta[i].Get<Plugin::Tag::TRACK_ID>(trackId)) {
            MEDIA_LOG_E("PrepareStreams failed to extract mime or trackId.");
            continue;
        }
        if (IsAudioMime(mime)) {
            MEDIA_LOG_D("PrepareStreams, audio stream with trackId = " PUBLIC_LOG_U32 ".", trackId);
            if (audioTrackCnt == 1) {
                MEDIA_LOG_E("PrepareStreams, discard audio track: " PUBLIC_LOG_D32 ".", trackId);
                continue;
            }
            ++audioTrackCnt;
        }
        auto port = std::make_shared<OutPort>(this, NamePort(mime));
        MEDIA_LOG_I("PrepareStreams, trackId: " PUBLIC_LOG_D32 ", portName: " PUBLIC_LOG_S,
                    i, port->GetName().c_str());
        outPorts_.push_back(port);
        portInfo.ports.push_back({port->GetName(), IsRawAudio(mime)});
        mediaMetaData_.trackInfos.emplace_back(trackId, std::move(port), true);
    }
    if (portInfo.ports.empty()) {
        MEDIA_LOG_E("PrepareStreams failed due to no valid port.");
        return false;
    }
    ErrorCode ret = ErrorCode::SUCCESS;
    if (callback_) {
        ret = callback_->OnCallback(FilterCallbackType::PORT_ADDED, static_cast<Filter*>(this), portInfo);
    }
    return ret == ErrorCode::SUCCESS;
}

ErrorCode DemuxerFilter::ReadFrame(AVBuffer& buffer, uint32_t& trackId)
{
    MEDIA_LOG_DD("ReadFrame called");
    ErrorCode result = ErrorCode::ERROR_UNKNOWN;
    auto rtv = plugin_->ReadFrame(buffer, 0);
    if (rtv == Plugin::Status::OK) {
        trackId = buffer.trackID;
        result = ErrorCode::SUCCESS;
    }
    MEDIA_LOG_DD("ReadFrame return with rtv = " PUBLIC_LOG_D32, static_cast<int32_t>(rtv));
    return (rtv != Plugin::Status::END_OF_STREAM) ? result : ErrorCode::END_OF_STREAM;
}

std::shared_ptr<Plugin::Meta> DemuxerFilter::GetTrackMeta(uint32_t trackId)
{
    uint32_t streamTrackId = 0;
    for (auto meta : mediaMetaData_.trackMetas) {
        auto ret = false;
        FALSE_LOG_MSG(ret = meta->Get<Plugin::Tag::TRACK_ID>(streamTrackId), "Get TRACK_ID failed.");
        if (ret && streamTrackId == trackId) {
            return meta;
        }
    }
    return nullptr;
}

void DemuxerFilter::SendEventEos()
{
    MEDIA_LOG_I("SendEventEos called");
    AVBufferPtr bufferPtr = std::make_shared<AVBuffer>();
    bufferPtr->flag = BUFFER_FLAG_EOS;
    for (const auto& stream : mediaMetaData_.trackInfos) {
        stream.port->PushData(bufferPtr, -1);
    }
}

void DemuxerFilter::HandleFrame(const AVBufferPtr& bufferPtr, uint32_t trackId)
{
    for (auto& stream : mediaMetaData_.trackInfos) {
        if (stream.trackId != trackId) {
            continue;
        }
        stream.port->PushData(bufferPtr, -1);
        break;
    }
}

void DemuxerFilter::UpdateStreamMeta(std::shared_ptr<Plugin::Meta>& streamMeta, Plugin::Capability& negotiatedCap,
    Plugin::Meta& downstreamParams)
{
    auto type = Plugin::MediaType::UNKNOWN;
    FALSE_LOG(streamMeta->Get<Plugin::Tag::MEDIA_TYPE>(type));
    if (type == Plugin::MediaType::AUDIO) {
        uint32_t channels = 2;
        uint32_t outputChannels = 2;
        Plugin::AudioChannelLayout channelLayout = Plugin::AudioChannelLayout::STEREO;
        Plugin::AudioChannelLayout outputChannelLayout = Plugin::AudioChannelLayout::STEREO;
        FALSE_LOG(streamMeta->Get<Plugin::Tag::AUDIO_CHANNELS>(channels));
        FALSE_LOG(streamMeta->Get<Plugin::Tag::AUDIO_CHANNEL_LAYOUT>(channelLayout));

        FALSE_LOG(downstreamParams.Get<Tag::AUDIO_OUTPUT_CHANNELS>(outputChannels));
        FALSE_LOG(downstreamParams.Get<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(outputChannelLayout));
        if (channels <= outputChannels) {
            outputChannels = channels;
            outputChannelLayout = channelLayout;
        }
        FALSE_LOG(streamMeta->Set<Plugin::Tag::AUDIO_OUTPUT_CHANNELS>(outputChannels));
        FALSE_LOG(streamMeta->Set<Plugin::Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(outputChannelLayout));
    } else if (type == Plugin::MediaType::VIDEO) {
        if (negotiatedCap.keys.count(Capability::Key::VIDEO_BIT_STREAM_FORMAT)) {
            auto vecVdBitStreamFormat = Plugin::AnyCast<std::vector<Plugin::VideoBitStreamFormat>>(
                negotiatedCap.keys[Capability::Key::VIDEO_BIT_STREAM_FORMAT]);
            if (!vecVdBitStreamFormat.empty()) {
                (void)plugin_->SetParameter(Tag::VIDEO_BIT_STREAM_FORMAT, vecVdBitStreamFormat[0]);
            }
        }
    }
}

void DemuxerFilter::NegotiateDownstream()
{
    PROFILE_BEGIN("NegotiateDownstream profile begins.");
    for (auto& stream : mediaMetaData_.trackInfos) {
        if (stream.needNegoCaps) {
            Capability caps;
            MEDIA_LOG_I("demuxer negotiate with trackId: " PUBLIC_LOG_U32, stream.trackId);
            auto streamMeta = GetTrackMeta(stream.trackId);
            auto tmpCap = MetaToCapability(*streamMeta);
            Plugin::Meta upstreamParams;
            Plugin::Meta downstreamParams;
            FALSE_LOG(upstreamParams.Set<Tag::MEDIA_SEEKABLE>(seekable_));
            if (stream.port->Negotiate(tmpCap, caps, upstreamParams, downstreamParams)) {
                UpdateStreamMeta(streamMeta, caps, downstreamParams);
                if (stream.port->Configure(streamMeta, upstreamParams, downstreamParams)) {
                    stream.needNegoCaps = false;
                }
            } else {
                task_->PauseAsync();
                MEDIA_LOG_E("NegotiateDownstream failed error.");
                OnEvent({name_, EventType::EVENT_ERROR, ErrorCode::ERROR_UNSUPPORTED_FORMAT});
            }
        }
    }
    PROFILE_END("NegotiateDownstream end.");
}

void DemuxerFilter::DemuxerLoop()
{
    if (pluginState_.load() == DemuxerState::DEMUXER_STATE_PARSE_FRAME) {
        AVBufferPtr bufferPtr = std::make_shared<AVBuffer>();
        uint32_t streamIndex = 0;
        auto rtv = ReadFrame(*bufferPtr, streamIndex);
        if (rtv == ErrorCode::SUCCESS) {
            DUMP_BUFFER2LOG("Demuxer Output", bufferPtr, 0);
            DUMP_BUFFER2FILE(DEMUXER_OUTPUT, bufferPtr);
            HandleFrame(bufferPtr, streamIndex);
        } else {
            SendEventEos();
            task_->PauseAsync();
            if (rtv != ErrorCode::END_OF_STREAM) {
                MEDIA_LOG_E("ReadFrame failed with rtv = " PUBLIC_LOG_D32, CppExt::to_underlying(rtv));
            }
        }
    } else {
        Plugin::MediaInfoHelper mediaInfo;
        PROFILE_BEGIN();
        if (plugin_->GetMediaInfo(mediaInfo) == Plugin::Status::OK && PrepareStreams(mediaInfo)) {
            PROFILE_END("Succeed to extract mediainfo.");
            NegotiateDownstream();
            ReportVideoSize(mediaInfo);
            pluginState_ = DemuxerState::DEMUXER_STATE_PARSE_FRAME;
            state_ = FilterState::READY;
            OnEvent({name_, EventType::EVENT_READY, {}});
        } else {
            task_->PauseAsync();
            MEDIA_LOG_E("demuxer filter parse meta failed");
            OnEvent({name_, EventType::EVENT_ERROR, ErrorCode::ERROR_UNKNOWN});
        }
    }
}

void DemuxerFilter::ReportVideoSize(const Plugin::MediaInfoHelper& mediaInfo)
{
    std::string mime;
    uint32_t width = 0;
    uint32_t height = 0;
    int streamCnt = mediaInfo.trackMeta.size();
    for (int i = 0; i < streamCnt; ++i) {
        if (!mediaInfo.trackMeta[i].Get<Plugin::Tag::MIME>(mime)) {
            MEDIA_LOG_E("PrepareStreams failed to extract mime.");
            continue;
        }
        if (IsVideoMime(mime)) {
            FALSE_LOG(mediaInfo.trackMeta[i].Get<Plugin::Tag::VIDEO_WIDTH>(width));
            FALSE_LOG(mediaInfo.trackMeta[i].Get<Plugin::Tag::VIDEO_HEIGHT>(height));
            MEDIA_LOG_I("mime-width-height: " PUBLIC_LOG_S "-" PUBLIC_LOG_U32 "-" PUBLIC_LOG_U32,
                        mime.c_str(), width, height);
            auto resolution = std::make_pair(static_cast<int32_t>(width), static_cast<int32_t>(height));
            FilterBase::OnEvent({name_, EventType::EVENT_RESOLUTION_CHANGE, resolution});
            break;
        }
    }
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
