/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "CodecFilterBase"

#include "pipeline/filters/codec/codec_filter_base.h"
#include "foundation/cpp_ext/memory_ext.h"
#include "foundation/utils/steady_clock.h"
#include "pipeline/filters/common/plugin_settings.h"
#include "plugin/common/plugin_attr_desc.h"

namespace {
constexpr uint32_t DEFAULT_OUT_BUFFER_POOL_SIZE = 5;
constexpr int32_t MAX_OUT_DECODED_DATA_SIZE_PER_FRAME = 20 * 1024; // 20kB
};

namespace OHOS {
namespace Media {
namespace Pipeline {
CodecFilterBase::CodecFilterBase(const std::string &name): FilterBase(name) {}

CodecFilterBase::~CodecFilterBase()
{
    MEDIA_LOG_D("codec filter base dtor called");
}

ErrorCode CodecFilterBase::Start()
{
    MEDIA_LOG_I("codec filter base start called");
    if (state_ != FilterState::READY && state_ != FilterState::PAUSED) {
        MEDIA_LOG_W("call decoder start() when state is not ready or working");
        return ErrorCode::ERROR_INVALID_OPERATION;
    }
    return FilterBase::Start();
}

ErrorCode CodecFilterBase::Stop()
{
    MEDIA_LOG_D("CodecFilterBase stop start.");
    // 先改变底层状态 然后停掉上层线程 否则会产生死锁
    FALSE_RETURN_V_MSG_W(plugin_!= nullptr, ErrorCode::ERROR_NULL_POINTER, "plugin is null");
    FAIL_RETURN_MSG(TranslatePluginStatus(plugin_->Flush()), "Flush plugin fail");
    FAIL_RETURN_MSG(TranslatePluginStatus(plugin_->Stop()), "Stop plugin fail");
    FAIL_RETURN_MSG(codecMode_->Stop(), "Codec mode stop fail");
    MEDIA_LOG_D("CodecFilterBase stop end.");
    return FilterBase::Stop();
}

ErrorCode CodecFilterBase::Prepare()
{
    MEDIA_LOG_I("codec filter base prepare called");
    if (state_ != FilterState::INITIALIZED) {
        MEDIA_LOG_W("codec filter is not in init state");
        return ErrorCode::ERROR_INVALID_OPERATION;
    }
    auto err = FilterBase::Prepare();
    FAIL_RETURN_MSG(err, "codec prepare error because of filter base prepare error");
    return err;
}

ErrorCode CodecFilterBase::UpdateMetaFromPlugin(Plugin::Meta& meta)
{
    auto parameterMap = PluginParameterTable::GetInstance().FindAllowedParameterMap(filterType_);
    for (const auto& paramMapping : parameterMap) {
        if ((paramMapping.second.second & PARAM_GET) == 0) {
            continue;
        }
        Plugin::ValueType tmpVal;
        auto ret = TranslatePluginStatus(plugin_->GetParameter(paramMapping.first, tmpVal));
        if (ret != ErrorCode::SUCCESS) {
            if (Plugin::HasTagInfo(paramMapping.first)) {
                MEDIA_LOG_I("GetParameter " PUBLIC_LOG_S " from plugin " PUBLIC_LOG_S "failed with code "
                    PUBLIC_LOG_D32, Plugin::GetTagStrName(paramMapping.first), pluginInfo_->name.c_str(), ret);
            } else {
                MEDIA_LOG_I("Tag " PUBLIC_LOG_D32 " is not is map, may be update it?", paramMapping.first);
                MEDIA_LOG_I("GetParameter " PUBLIC_LOG_D32 " from plugin " PUBLIC_LOG_S " failed with code "
                    PUBLIC_LOG_D32, paramMapping.first, pluginInfo_->name.c_str(), ret);
            }
            continue;
        }
        if (!paramMapping.second.first(paramMapping.first, tmpVal)) {
            if (Plugin::HasTagInfo(paramMapping.first)) {
                MEDIA_LOG_I("Type of Tag " PUBLIC_LOG_S " should be " PUBLIC_LOG_S,
                    Plugin::GetTagStrName(paramMapping.first), std::get<2>(Plugin::g_tagInfoMap.at(paramMapping.first)));
            } else {
                MEDIA_LOG_I("Tag " PUBLIC_LOG_D32 " is not is map, may be update it?", paramMapping.first);
                MEDIA_LOG_I("Type of Tag " PUBLIC_LOG_D32 "mismatch", paramMapping.first);
            }
            continue;
        }
        meta.SetData(static_cast<Plugin::Tag>(paramMapping.first), tmpVal);
    }
    return ErrorCode::SUCCESS;
}

ErrorCode CodecFilterBase::SetPluginParameterLocked(Tag tag, const Plugin::ValueType& value)
{
    return TranslatePluginStatus(plugin_->SetParameter(tag, value));
}

ErrorCode CodecFilterBase::SetParameter(int32_t key, const Plugin::Any& value)
{
    if (state_.load() == FilterState::CREATED) {
        return ErrorCode::ERROR_AGAIN;
    }
    Tag tag = Tag::INVALID;
    FALSE_RETURN_V_MSG_E(TranslateIntoParameter(key, tag), ErrorCode::ERROR_INVALID_PARAMETER_VALUE,
                         "key " PUBLIC_LOG_D32 " is out of boundary", key);
    RETURN_AGAIN_IF_NULL(plugin_);
    return SetPluginParameterLocked(tag, value);
}

ErrorCode CodecFilterBase::GetParameter(int32_t key, Plugin::Any& outVal)
{
    if (state_.load() == FilterState::CREATED) {
        return ErrorCode::ERROR_AGAIN;
    }
    Tag tag = Tag::INVALID;
    FALSE_RETURN_V_MSG_E(TranslateIntoParameter(key, tag), ErrorCode::ERROR_INVALID_PARAMETER_VALUE,
                      "key " PUBLIC_LOG_D32 " is out of boundary", key);
    RETURN_AGAIN_IF_NULL(plugin_);
    return TranslatePluginStatus(plugin_->GetParameter(tag, outVal));
}

void CodecFilterBase::UpdateParams(const std::shared_ptr<const Plugin::Meta>& upMeta,
                                   std::shared_ptr<Plugin::Meta>& meta)
{
}

std::shared_ptr<Allocator> CodecFilterBase::GetAllocator()
{
    return plugin_->GetAllocator();
}

uint32_t CodecFilterBase::GetOutBufferPoolSize()
{
    return DEFAULT_OUT_BUFFER_POOL_SIZE;
}

uint32_t CodecFilterBase::CalculateBufferSize(const std::shared_ptr<const Plugin::Meta>& meta)
{
    return 0;
}

Plugin::Meta CodecFilterBase::GetNegotiateParams(const Plugin::Meta& upstreamParams)
{
    return upstreamParams;
}

bool CodecFilterBase::CheckRequiredOutCapKeys(const Capability& capability)
{
    std::vector<Capability::Key> outCapKeys = GetRequiredOutCapKeys();
    for (auto outCapKey : outCapKeys) {
        if (capability.keys.count(outCapKey) == 0) {
            MEDIA_LOG_W("decoder plugin must specify key " PUBLIC_LOG_S " in out caps",
                        Plugin::Tag2String(static_cast<Plugin::Tag>(outCapKey)));
            return false;
        }
    }
    return true;
}

std::vector<Capability::Key> CodecFilterBase::GetRequiredOutCapKeys()
{
    // Not required by default
    return {};
}

bool CodecFilterBase::Negotiate(const std::string& inPort,
                                const std::shared_ptr<const Plugin::Capability>& upstreamCap,
                                Plugin::Capability& negotiatedCap,
                                const Plugin::Meta& upstreamParams,
                                Plugin::Meta& downstreamParams)
{
    PROFILE_BEGIN("CodecFilterBase negotiate start");
    FALSE_RETURN_V_MSG_W(state_ == FilterState::PREPARING, false, "filter is not preparing when negotiate");
    auto targetOutPort = GetRouteOutPort(inPort);
    FALSE_RETURN_V_MSG_W(targetOutPort != nullptr, false, "codec outPort is not found");
    std::shared_ptr<Plugin::PluginInfo> selectedPluginInfo = nullptr;
    bool atLeastOutCapMatched = false;
    auto candidatePlugins = FindAvailablePlugins(*upstreamCap, pluginType_, preferredCodecMode_);
    for (const auto& candidate : candidatePlugins) {
        FALSE_LOG_MSG(!candidate.first->outCaps.empty(),
                      "plugin " PUBLIC_LOG_S " has no out caps", candidate.first->name.c_str());
        for (const auto& outCap : candidate.first->outCaps) { // each codec plugin should have at least one out cap
            if (!CheckRequiredOutCapKeys(outCap)) {
                continue;
            }
            auto thisOut = std::make_shared<Plugin::Capability>();
            if (!MergeCapabilityKeys(*upstreamCap, outCap, *thisOut)) {
                MEDIA_LOG_W("one cap of plugin " PUBLIC_LOG_S " mismatch upstream cap", candidate.first->name.c_str());
                continue;
            }
            atLeastOutCapMatched = true;
            thisOut->mime = outCap.mime;
            Plugin::Meta proposeParams;
            if (targetOutPort->Negotiate(thisOut, capNegWithDownstream_, proposeParams, downstreamParams)) {
                negotiatedCap = candidate.second;
                selectedPluginInfo = candidate.first;
                MEDIA_LOG_I("use plugin " PUBLIC_LOG_S, candidate.first->name.c_str());
                MEDIA_LOG_I("neg upstream cap " PUBLIC_LOG_S, Capability2String(negotiatedCap).c_str());
                MEDIA_LOG_I("neg downstream cap " PUBLIC_LOG_S, Capability2String(capNegWithDownstream_).c_str());
                break;
            }
        }
        if (selectedPluginInfo != nullptr) { // select the first one
            break;
        }
    }
    FALSE_RETURN_V_MSG_E(atLeastOutCapMatched && selectedPluginInfo != nullptr, false,
        "can't find available codec plugin with " PUBLIC_LOG_S, Capability2String(*upstreamCap).c_str());

    auto res = UpdateAndInitPluginByInfo<Plugin::Codec>(plugin_, pluginInfo_, selectedPluginInfo,
        [this](const std::string& name)-> std::shared_ptr<Plugin::Codec> {
            return Plugin::PluginManager::Instance().CreateCodecPlugin(name, pluginType_);
    });
    FALSE_RETURN_V(codecMode_->Init(plugin_, outPorts_), false);
    PROFILE_END("async codec negotiate end");
    MEDIA_LOG_D("codec filter base negotiate end");
    return res;
}

bool CodecFilterBase::Configure(const std::string& inPort, const std::shared_ptr<const Plugin::Meta>& upstreamMeta,
                                Plugin::Meta& upstreamParams, Plugin::Meta& downstreamParams)
{
    MEDIA_LOG_I("receive upstream meta " PUBLIC_LOG_S, Meta2String(*upstreamMeta).c_str());
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr && pluginInfo_ != nullptr, false,
                         "can't configure codec when no plugin available");
    auto thisMeta = std::make_shared<Plugin::Meta>();
    FALSE_RETURN_V_MSG_E(MergeMetaWithCapability(*upstreamMeta, capNegWithDownstream_, *thisMeta), false,
                         "can't configure codec plugin since meta is not compatible with negotiated caps");
    UpdateParams(upstreamMeta, thisMeta);

    // When use hdi as codec plugin interfaces, must set width & height into hdi,
    // Hdi use these params to calc out buffer size & count then return to filter
    if (ConfigPluginWithMeta(*plugin_, *thisMeta) != ErrorCode::SUCCESS) {
        MEDIA_LOG_E("set params into plugin failed");
        return false;
    }
    uint32_t bufferCnt = 0;
    if (GetPluginParameterLocked(Tag::REQUIRED_OUT_BUFFER_CNT, bufferCnt) != ErrorCode::SUCCESS) {
        bufferCnt = GetOutBufferPoolSize();
    }
    MEDIA_LOG_D("bufferCnt: " PUBLIC_LOG_U32, bufferCnt);

    upstreamParams.Set<Plugin::Tag::VIDEO_MAX_SURFACE_NUM>(bufferCnt);
    auto targetOutPort = GetRouteOutPort(inPort);
    if (targetOutPort == nullptr || !targetOutPort->Configure(thisMeta, upstreamParams, downstreamParams)) {
        MEDIA_LOG_E("decoder filter downstream Configure failed");
        return false;
    }
    sinkParams_ = downstreamParams;
    uint32_t bufferSize = 0;
    if (GetPluginParameterLocked(Tag::REQUIRED_OUT_BUFFER_SIZE, bufferSize) != ErrorCode::SUCCESS) {
        bufferSize = CalculateBufferSize(thisMeta);
        if (bufferSize == 0) {
            bufferSize = MAX_OUT_DECODED_DATA_SIZE_PER_FRAME;
        }
    }
    std::shared_ptr<Allocator> outAllocator = GetAllocator();
    codecMode_->CreateOutBufferPool(outAllocator, bufferCnt, bufferSize, bufferMetaType_);
    MEDIA_LOG_D("AllocateOutputBuffers success");
    auto err = ConfigureToStartPluginLocked(thisMeta);
    if (err != ErrorCode::SUCCESS) {
        MEDIA_LOG_E("CodecFilterBase configure error");
        OnEvent({name_, EventType::EVENT_ERROR, err});
        return false;
    }
    state_ = FilterState::READY;
    OnEvent({name_, EventType::EVENT_READY});
    MEDIA_LOG_I("CodecFilterBase send EVENT_READY");
    return true;
}

ErrorCode CodecFilterBase::ConfigureToStartPluginLocked(const std::shared_ptr<const Plugin::Meta>& meta)
{
    MEDIA_LOG_D("CodecFilterBase configure called");
    FAIL_RETURN_MSG(TranslatePluginStatus(plugin_->SetCallback(this)), "plugin set callback fail");
    FAIL_RETURN_MSG(TranslatePluginStatus(plugin_->SetDataCallback(this)), "plugin set data callback fail");
    FAIL_RETURN_MSG(codecMode_->Configure(), "codec mode configure error");
    return ErrorCode::SUCCESS;
}

void CodecFilterBase::FlushStart()
{
    MEDIA_LOG_D("FlushStart entered.");
    isFlushing_ = true;
    if (plugin_ != nullptr) {
        auto err = TranslatePluginStatus(plugin_->Flush());
        if (err != ErrorCode::SUCCESS) {
            MEDIA_LOG_E("codec plugin flush error");
        }
    }
    MEDIA_LOG_D("FlushStart exit.");
}

void CodecFilterBase::FlushEnd()
{
    MEDIA_LOG_I("FlushEnd entered");
    isFlushing_ = false;
}

ErrorCode CodecFilterBase::PushData(const std::string &inPort, const AVBufferPtr& buffer, int64_t offset)
{
    if (state_ != FilterState::READY && state_ != FilterState::PAUSED && state_ != FilterState::RUNNING) {
        MEDIA_LOG_W("pushing data to decoder when state is " PUBLIC_LOG_D32, static_cast<int>(state_.load()));
        return ErrorCode::ERROR_INVALID_OPERATION;
    }
    if (isFlushing_) {
        MEDIA_LOG_I("Flushing, discarding this data from port " PUBLIC_LOG_S, inPort.c_str());
        return ErrorCode::SUCCESS;
    }
    return codecMode_->PushData(inPort, buffer, offset);
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
