/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "event_aggregate.h"
#include "log.h"
#include "media_monitor_info.h"
#include "monitor_utils.h"
#include "audio_system_manager.h"
#include "audio_device_info.h"

#include "bundle_mgr_interface.h"
#include "bundle_mgr_proxy.h"
#include "iservice_registry.h"

using OHOS::AudioStandard::AudioSystemManager;

namespace OHOS {
namespace Media {
namespace MediaMonitor {

static constexpr int32_t NEED_INCREASE_FREQUENCY = 30;
static constexpr int32_t UNINITIALIZED = -1;

static AppExecFwk::BundleInfo GetBundleInfoFromUid(int32_t appUid)
{
    std::string bundleName {""};
    AppExecFwk::BundleInfo bundleInfo;
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    FALSE_RETURN_V_MSG_E(systemAbilityManager != nullptr, bundleInfo, "systemAbilityManager is nullptr");

    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    FALSE_RETURN_V_MSG_E(remoteObject != nullptr, bundleInfo, "remoteObject is nullptr");

    sptr<AppExecFwk::IBundleMgr> bundleMgrProxy = OHOS::iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
    FALSE_RETURN_V_MSG_E(bundleMgrProxy != nullptr, bundleInfo, "bundleMgrProxy is nullptr");

    bundleMgrProxy->GetNameForUid(appUid, bundleName);

    bundleMgrProxy->GetBundleInfoV9(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT |
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_ABILITIES |
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_REQUESTED_PERMISSION |
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_EXTENSION_INFO |
        AppExecFwk::BundleFlag::GET_BUNDLE_WITH_HASH_VALUE,
        bundleInfo,
        AppExecFwk::Constants::ALL_USERID);

    return bundleInfo;
}

EventAggregate::EventAggregate()
    :audioMemo_(AudioMemo::GetAudioMemo()),
    mediaMonitorPolicy_(MediaMonitorPolicy::GetMediaMonitorPolicy())
{
    MEDIA_LOG_D("EventAggregate Constructor");
}

EventAggregate::~EventAggregate()
{
    MEDIA_LOG_D("EventAggregate Destructor");
}

void EventAggregate::WriteEvent(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("WriteEvent enter");
    if (bean == nullptr) {
        MEDIA_LOG_E("eventBean is nullptr");
        return;
    }

    EventId eventId = bean->GetEventId();
    switch (eventId) {
        case HEADSET_CHANGE:
        case AUDIO_ROUTE_CHANGE:
        case LOAD_CONFIG_ERROR:
        case AUDIO_SERVICE_STARTUP_ERROR:
            mediaMonitorPolicy_.WriteEvent(bean->GetEventId(), bean);
            break;
        default:
            UpdateAggregateEventList(bean);
            break;
    }
}

void EventAggregate::UpdateAggregateEventList(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Update Aggregate Event List");
    EventId eventId = bean->GetEventId();
    switch (eventId) {
        case DEVICE_CHANGE:
            HandleDeviceChangeEvent(bean);
            break;
        case STREAM_CHANGE:
            HandleStreamChangeEvent(bean);
            break;
        case AUDIO_STREAM_EXHAUSTED_STATS:
            HandleStreamExhaustedErrorEvent(bean);
            break;
        case AUDIO_STREAM_CREATE_ERROR_STATS:
            HandleStreamCreateErrorEvent(bean);
            break;
        case BACKGROUND_SILENT_PLAYBACK:
            HandleBackgroundSilentPlayback(bean);
            break;
        case PERFORMANCE_UNDER_OVERRUN_STATS:
            HandleUnderrunStatistic(bean);
            break;
        case SET_FORCE_USE_AUDIO_DEVICE:
            HandleForceUseDevice(bean);
            break;
        case CAPTURE_MUTE_STATUS_CHANGE:
            HandleCaptureMutedStatusChange(bean);
            break;
        case VOLUME_CHANGE:
            HandleVolumeChange(bean);
            break;
        case AUDIO_PIPE_CHANGE:
            HandlePipeChange(bean);
            break;
        case AUDIO_FOCUS_MIGRATE:
            HandleFocusMigrate(bean);
            break;
        default:
            break;
    }
}

void EventAggregate::HandleDeviceChangeEvent(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Begin handle device change event");
    HandleDeviceChangeForDeviceUsage(bean);
    HandleDeviceChangeForCaptureMuted(bean);
    HandleDeviceChangeForVolume(bean);
    mediaMonitorPolicy_.WriteEvent(bean->GetEventId(), bean);
}

void EventAggregate::HandleDeviceChangeForDeviceUsage(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle device change for device event aggregate.");
    auto isExist = [&bean](const std::shared_ptr<EventBean> &deviceUsageBean) {
        if (bean->GetIntValue("ISOUTPUT") == deviceUsageBean->GetIntValue("IS_PLAYBACK") &&
            bean->GetIntValue("STREAMID") == deviceUsageBean->GetIntValue("STREAMID") &&
            bean->GetIntValue("STREAMTYPE") == deviceUsageBean->GetIntValue("STREAM_TYPE")) {
            MEDIA_LOG_D("Find the existing device usage");
            return true;
        }
        return false;
    };
    auto it = std::find_if(deviceUsageVector_.begin(), deviceUsageVector_.end(), isExist);
    if (it != deviceUsageVector_.end()) {
        HandleDeviceChangeForDuration(FOR_DEVICE_EVENT, bean, *it);
    }
}

void EventAggregate::HandleDeviceChangeForCaptureMuted(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle device change for capture muted event aggregate");
    if (bean->GetIntValue("ISOUTPUT")) {
        MEDIA_LOG_W("HandleDeviceChangeForCaptureMuted is playback");
        return;
    }
    if (!bean->GetIntValue("MUTED")) {
        MEDIA_LOG_W("HandleDeviceChangeForCaptureMuted is not muted");
        return;
    }
    auto isExist = [&bean](const std::shared_ptr<EventBean> &captureMutedBean) {
        if (bean->GetIntValue("STREAMID") == captureMutedBean->GetIntValue("STREAMID") &&
            bean->GetIntValue("STREAM_TYPE") == captureMutedBean->GetIntValue("STREAM_TYPE")) {
            MEDIA_LOG_D("Find the existing capture muted");
            return true;
        }
        return false;
    };
    auto it = std::find_if(captureMutedVector_.begin(), captureMutedVector_.end(), isExist);
    if (it != captureMutedVector_.end()) {
        HandleDeviceChangeForDuration(FOR_CAPTURE_MUTE_EVENT, bean, *it);
    }
}

void EventAggregate::HandleDeviceChangeForVolume(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle device change for volume event aggregate");
    if (!bean->GetIntValue("ISOUTPUT")) {
        MEDIA_LOG_D("HandleDeviceChangeForVolume is not playback");
        return;
    }
    auto isExist = [&bean](const std::shared_ptr<EventBean> &volumeBean) {
        if (bean->GetIntValue("STREAMID") == volumeBean->GetIntValue("STREAMID")) {
            MEDIA_LOG_D("Find the existing volume");
            return true;
        }
        return false;
    };
    auto it = std::find_if(volumeVector_.begin(), volumeVector_.end(), isExist);
    if (it != volumeVector_.end()) {
        HandleDeviceChangeForDuration(FOR_VOLUME_CHANGE_EVENT, bean, *it);
    }
}

void EventAggregate::HandleDeviceChangeForDuration(const DeviceChangeEvent &event,
    std::shared_ptr<EventBean> &bean, std::shared_ptr<EventBean> &beanInVector)
{
    if (bean->GetIntValue("DEVICETYPE") != beanInVector->GetIntValue("DEVICE_TYPE")) {
        int64_t duration = TimeUtils::GetCurSec() - beanInVector->GetUint64Value("START_TIME");
        if (duration > 0 && duration > NEED_INCREASE_FREQUENCY) {
            beanInVector->Add("DURATION", static_cast<uint64_t>(duration));
            if (event == FOR_DEVICE_EVENT) {
                mediaMonitorPolicy_.HandDeviceUsageToEventVector(beanInVector);
            } else if (event == FOR_CAPTURE_MUTE_EVENT) {
                mediaMonitorPolicy_.HandleCaptureMutedToEventVector(beanInVector);
            } else if (event == FOR_VOLUME_CHANGE_EVENT) {
                mediaMonitorPolicy_.HandleVolumeToEventVector(beanInVector);
            }
            mediaMonitorPolicy_.WhetherToHiSysEvent();
        }
        beanInVector->UpdateIntMap("DEVICE_TYPE", bean->GetIntValue("DEVICETYPE"));
        beanInVector->UpdateUint64Map("START_TIME", TimeUtils::GetCurSec());
    }
}

void EventAggregate::HandleStreamChangeEvent(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle stream change event");
    if (bean->GetIntValue("STATE") == AudioStandard::State::RUNNING) {
        MEDIA_LOG_D("Stream State RUNNING");
        uint64_t curruntTime = TimeUtils::GetCurSec();
        AppExecFwk::BundleInfo bundleInfo = GetBundleInfoFromUid(bean->GetIntValue("UID"));
        bean->Add("APP_NAME", bundleInfo.name);
        AddToDeviceUsage(bean, curruntTime);
        AddToStreamUsage(bean, curruntTime);
        AddToCaptureMuteUsage(bean, curruntTime);
        AddToVolumeVector(bean, curruntTime);
    } else if (bean->GetIntValue("STATE") == AudioStandard::State::STOPPED ||
                bean->GetIntValue("STATE") == AudioStandard::State::PAUSED ||
                bean->GetIntValue("STATE") == AudioStandard::State::RELEASED) {
        MEDIA_LOG_D("Stream State STOPPED/PAUSED/RELEASED");
        AppExecFwk::BundleInfo bundleInfo = GetBundleInfoFromUid(bean->GetIntValue("UID"));
        bean->Add("APP_NAME", bundleInfo.name);
        HandleDeviceUsage(bean);
        HandleStreamUsage(bean);
        HandleCaptureMuted(bean);
        HandleStreamChangeForVolume(bean);
    }
    mediaMonitorPolicy_.WriteEvent(bean->GetEventId(), bean);
}

void EventAggregate::AddToDeviceUsage(std::shared_ptr<EventBean> &bean, uint64_t curruntTime)
{
    MEDIA_LOG_D("Add to device usage from stream change event");
    auto isExist = [&bean](const std::shared_ptr<EventBean> &eventBean) {
        if (bean->GetIntValue("ISOUTPUT") == eventBean->GetIntValue("IS_PLAYBACK") &&
            bean->GetIntValue("STREAMID") == eventBean->GetIntValue("STREAMID") &&
            bean->GetIntValue("UID") == eventBean->GetIntValue("UID") &&
            bean->GetIntValue("PID") == eventBean->GetIntValue("PID") &&
            bean->GetIntValue("STREAM_TYPE") == eventBean->GetIntValue("STREAM_TYPE") &&
            bean->GetIntValue("STATE") == eventBean->GetIntValue("STATE") &&
            bean->GetIntValue("DEVICETYPE") == eventBean->GetIntValue("DEVICE_TYPE")) {
            MEDIA_LOG_D("Find the existing device usage");
            return true;
        }
        return false;
    };
    auto it = std::find_if(deviceUsageVector_.begin(), deviceUsageVector_.end(), isExist);
    if (it != deviceUsageVector_.end()) {
        MEDIA_LOG_D("The current device already exists, do not add it again");
        return;
    }
    std::shared_ptr<EventBean> deviceUsageBean = std::make_shared<EventBean>();
    int32_t deviceType = bean->GetIntValue("DEVICETYPE");
    deviceUsageBean->Add("IS_PLAYBACK", bean->GetIntValue("ISOUTPUT"));
    deviceUsageBean->Add("STREAMID", bean->GetIntValue("STREAMID"));
    deviceUsageBean->Add("UID", bean->GetIntValue("UID"));
    deviceUsageBean->Add("PID", bean->GetIntValue("PID"));
    deviceUsageBean->Add("STREAM_TYPE", bean->GetIntValue("STREAM_TYPE"));
    deviceUsageBean->Add("STATE", bean->GetIntValue("STATE"));
    deviceUsageBean->Add("DEVICE_TYPE", deviceType);
    deviceUsageBean->Add("START_TIME", curruntTime);
    deviceUsageVector_.push_back(deviceUsageBean);
    if (deviceType == AudioStandard::DEVICE_TYPE_BLUETOOTH_SCO ||
        deviceType == AudioStandard::DEVICE_TYPE_BLUETOOTH_A2DP) {
        deviceUsageBean->Add("BT_TYPE", bean->GetIntValue("BT_TYPE"));
    }
}

void EventAggregate::AddToStreamUsage(std::shared_ptr<EventBean> &bean, uint64_t curruntTime)
{
    MEDIA_LOG_D("Add to stream usage from stream change event");
    auto isExist = [&bean](const std::shared_ptr<EventBean> &eventBean) {
        if (bean->GetIntValue("STREAMID") == eventBean->GetIntValue("STREAMID") &&
            bean->GetIntValue("UID") == eventBean->GetIntValue("UID") &&
            bean->GetIntValue("PID") == eventBean->GetIntValue("PID") &&
            bean->GetIntValue("STREAM_TYPE") == eventBean->GetIntValue("STREAM_TYPE") &&
            bean->GetIntValue("DEVICETYPE") == eventBean->GetIntValue("DEVICE_TYPE") &&
            bean->GetIntValue("ISOUTPUT") == eventBean->GetIntValue("IS_PLAYBACK") &&
            bean->GetIntValue("PIPE_TYPE") == eventBean->GetIntValue("PIPE_TYPE") &&
            bean->GetIntValue("SAMPLE_RATE") == eventBean->GetIntValue("SAMPLE_RATE")) {
            MEDIA_LOG_D("Find the existing stream usage");
            return true;
        }
        return false;
    };
    auto it = std::find_if(streamUsageVector_.begin(), streamUsageVector_.end(), isExist);
    if (it != streamUsageVector_.end()) {
        MEDIA_LOG_D("The current stream already exists, do not add it again");
        return;
    }
    std::shared_ptr<EventBean> streamUsageBean = std::make_shared<EventBean>();
    streamUsageBean->Add("STREAMID", bean->GetIntValue("STREAMID"));
    streamUsageBean->Add("UID", bean->GetIntValue("UID"));
    streamUsageBean->Add("PID", bean->GetIntValue("PID"));
    streamUsageBean->Add("STREAM_TYPE", bean->GetIntValue("STREAM_TYPE"));
    streamUsageBean->Add("DEVICE_TYPE", bean->GetIntValue("DEVICETYPE"));
    streamUsageBean->Add("IS_PLAYBACK", bean->GetIntValue("ISOUTPUT"));
    streamUsageBean->Add("PIPE_TYPE", bean->GetIntValue("PIPE_TYPE"));
    streamUsageBean->Add("SAMPLE_RATE", bean->GetIntValue("SAMPLE_RATE"));
    streamUsageBean->Add("APP_NAME", bean->GetStringValue("APP_NAME"));
    streamUsageBean->Add("STATE", bean->GetIntValue("STATE"));
    streamUsageBean->Add("START_TIME", curruntTime);
    streamUsageVector_.push_back(streamUsageBean);
}

void EventAggregate::AddToCaptureMuteUsage(std::shared_ptr<EventBean> &bean, uint64_t curruntTime)
{
    MEDIA_LOG_D("Add to capture mute usage from stream change event");
    if (bean->GetIntValue("ISOUTPUT")) {
        MEDIA_LOG_D("AddToCaptureMuteUsage is playback");
        return;
    }
    if (!bean->GetIntValue("MUTED")) {
        MEDIA_LOG_D("AddToCaptureMuteUsage is not muted");
        return;
    }
    auto isExist = [&bean](const std::shared_ptr<EventBean> &eventBean) {
        if (bean->GetIntValue("STREAMID") == eventBean->GetIntValue("STREAMID") &&
            bean->GetIntValue("STREAM_TYPE") == eventBean->GetIntValue("STREAM_TYPE") &&
            bean->GetIntValue("DEVICETYPE") == eventBean->GetIntValue("DEVICE_TYPE")) {
            MEDIA_LOG_D("Find the existing capture muted usage");
            return true;
        }
        return false;
    };
    auto it = std::find_if(captureMutedVector_.begin(), captureMutedVector_.end(), isExist);
    if (it != captureMutedVector_.end()) {
        MEDIA_LOG_D("The current capture already exists, do not add it again");
        return;
    }
    std::shared_ptr<EventBean> captureMutedBean = std::make_shared<EventBean>();
    captureMutedBean->Add("STREAMID", bean->GetIntValue("STREAMID"));
    captureMutedBean->Add("STREAM_TYPE", bean->GetIntValue("STREAM_TYPE"));
    captureMutedBean->Add("DEVICE_TYPE", bean->GetIntValue("DEVICETYPE"));
    captureMutedBean->Add("START_TIME", curruntTime);
    captureMutedVector_.push_back(captureMutedBean);
}

void EventAggregate::AddToVolumeVector(std::shared_ptr<EventBean> &bean, uint64_t curruntTime)
{
    MEDIA_LOG_D("Add to volume vector from stream change event");
    if (!bean->GetIntValue("ISOUTPUT")) {
        MEDIA_LOG_D("EventAggregate AddToVolumeVector is not playback");
        return;
    }
    auto isExist = [&bean](const std::shared_ptr<EventBean> &volumeBean) {
        if (bean->GetIntValue("STREAMID") == volumeBean->GetIntValue("STREAMID")) {
            MEDIA_LOG_D("Find the existing capture volume vector");
            return true;
        }
        return false;
    };
    bool isInVolumeMap = false;
    auto it = std::find_if(volumeVector_.begin(), volumeVector_.end(), isExist);
    if (it != volumeVector_.end()) {
        uint64_t curruntTime = TimeUtils::GetCurSec();
        if ((*it)->GetIntValue("DEVICE_TYPE") == UNINITIALIZED) {
            (*it)->UpdateIntMap("DEVICE_TYPE", bean->GetIntValue("DEVICETYPE"));
        } else {
            (*it)->Add("DEVICE_TYPE",  bean->GetIntValue("DEVICETYPE"));
        }
        if ((*it)->GetIntValue("STREAM_TYPE") == UNINITIALIZED) {
            (*it)->UpdateIntMap("STREAM_TYPE", bean->GetIntValue("STREAM_TYPE"));
        } else {
            (*it)->Add("STREAM_TYPE",  bean->GetIntValue("STREAM_TYPE"));
        }
        (*it)->Add("START_TIME", curruntTime);
        isInVolumeMap = true;
    }
    if (!isInVolumeMap) {
        std::shared_ptr<EventBean> volumeBean = std::make_shared<EventBean>();
        volumeBean->Add("STREAMID", bean->GetIntValue("STREAMID"));
        volumeBean->Add("STREAM_TYPE", bean->GetIntValue("STREAM_TYPE"));
        volumeBean->Add("DEVICE_TYPE", bean->GetIntValue("DEVICETYPE"));
        volumeBean->Add("LEVEL", systemVol_);
        volumeBean->Add("START_TIME", curruntTime);
        volumeVector_.push_back(volumeBean);
    }
}

void EventAggregate::HandleDeviceUsage(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle device usage");
    auto isExist = [&bean](const std::shared_ptr<EventBean> &deviceUsageBean) {
        if (bean->GetIntValue("STREAMID") == deviceUsageBean->GetIntValue("STREAMID") &&
            bean->GetIntValue("UID") == deviceUsageBean->GetIntValue("UID") &&
            bean->GetIntValue("PID") == deviceUsageBean->GetIntValue("PID") &&
            bean->GetIntValue("STREAM_TYPE") == deviceUsageBean->GetIntValue("STREAM_TYPE") &&
            bean->GetIntValue("DEVICETYPE") == deviceUsageBean->GetIntValue("DEVICE_TYPE") &&
            bean->GetIntValue("ISOUTPUT") == deviceUsageBean->GetIntValue("IS_PLAYBACK")) {
            MEDIA_LOG_D("Find the existing device usage");
            return true;
        }
        return false;
    };
    auto it = std::find_if(deviceUsageVector_.begin(), deviceUsageVector_.end(), isExist);
    if (it != deviceUsageVector_.end()) {
        int64_t duration = TimeUtils::GetCurSec() - (*it)->GetUint64Value("START_TIME");
        int32_t deviceType = (*it)->GetIntValue("DEVICE_TYPE");
        if (duration > 0 && duration > NEED_INCREASE_FREQUENCY) {
            (*it)->Add("DURATION", static_cast<uint64_t>(duration));
            mediaMonitorPolicy_.HandDeviceUsageToEventVector(*it);
            if (deviceType == AudioStandard::DEVICE_TYPE_BLUETOOTH_SCO ||
                deviceType == AudioStandard::DEVICE_TYPE_BLUETOOTH_A2DP) {
                mediaMonitorPolicy_.HandBtUsageToEventVector(*it);
            }
            mediaMonitorPolicy_.WhetherToHiSysEvent();
        }
        deviceUsageVector_.erase(it);
    }
}

void EventAggregate::HandleStreamUsage(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle stream usage");
    auto isExist = [&bean](const std::shared_ptr<EventBean> &streamUsageBean) {
        if (bean->GetIntValue("STREAMID") == streamUsageBean->GetIntValue("STREAMID") &&
            bean->GetIntValue("UID") == streamUsageBean->GetIntValue("UID") &&
            bean->GetIntValue("PID") == streamUsageBean->GetIntValue("PID") &&
            bean->GetIntValue("STREAM_TYPE") == streamUsageBean->GetIntValue("STREAM_TYPE") &&
            bean->GetIntValue("ISOUTPUT") == streamUsageBean->GetIntValue("IS_PLAYBACK") &&
            bean->GetIntValue("PIPE_TYPE") == streamUsageBean->GetIntValue("PIPE_TYPE") &&
            bean->GetIntValue("SAMPLE_RATE") == streamUsageBean->GetIntValue("SAMPLE_RATE")) {
            MEDIA_LOG_D("Find the existing stream usage");
            return true;
        }
        return false;
    };
    auto it = std::find_if(streamUsageVector_.begin(), streamUsageVector_.end(), isExist);
    if (it != streamUsageVector_.end()) {
        int64_t duration = TimeUtils::GetCurSec() - (*it)->GetUint64Value("START_TIME");
        if (duration > 0 && duration > NEED_INCREASE_FREQUENCY) {
            (*it)->Add("DURATION", static_cast<uint64_t>(duration));
            mediaMonitorPolicy_.HandStreamUsageToEventVector(*it);
            mediaMonitorPolicy_.WhetherToHiSysEvent();
        }
        streamUsageVector_.erase(it);
    }
}

void EventAggregate::HandleCaptureMuted(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle capture muted");
    if (bean->GetIntValue("ISOUTPUT")) {
        MEDIA_LOG_D("HandleCaptureMuted is playback");
        return;
    }
    auto isExist = [&bean](const std::shared_ptr<EventBean> &captureMutedBean) {
        if (bean->GetIntValue("STREAMID") == captureMutedBean->GetIntValue("STREAMID") &&
            bean->GetIntValue("STREAM_TYPE") == captureMutedBean->GetIntValue("STREAM_TYPE") &&
            bean->GetIntValue("DEVICETYPE") == captureMutedBean->GetIntValue("DEVICE_TYPE")) {
            MEDIA_LOG_D("Find the existing capture muted");
            return true;
        }
        return false;
    };
    auto it = std::find_if(captureMutedVector_.begin(), captureMutedVector_.end(), isExist);
    if (it != captureMutedVector_.end()) {
        int64_t duration = TimeUtils::GetCurSec() - (*it)->GetUint64Value("START_TIME");
        if ((*it)->GetIntValue("MUTED") && duration > 0 && duration > NEED_INCREASE_FREQUENCY) {
            (*it)->Add("DURATION", static_cast<uint64_t>(duration));
            mediaMonitorPolicy_.HandleCaptureMutedToEventVector(*it);
            mediaMonitorPolicy_.WhetherToHiSysEvent();
        }
        captureMutedVector_.erase(it);
    }
}

void EventAggregate::HandleStreamChangeForVolume(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle stream Change for volume");
    auto isExist = [&bean](const std::shared_ptr<EventBean> &volumeBean) {
        if (bean->GetIntValue("ISOUTPUT") &&
            bean->GetIntValue("STREAMID") == volumeBean->GetIntValue("STREAMID") &&
            bean->GetIntValue("STREAM_TYPE") == volumeBean->GetIntValue("STREAM_TYPE") &&
            bean->GetIntValue("DEVICETYPE") == volumeBean->GetIntValue("DEVICE_TYPE")) {
            MEDIA_LOG_D("Find the existing volume vector");
            return true;
        }
        return false;
    };

    auto it = std::find_if(volumeVector_.begin(), volumeVector_.end(), isExist);
    if (it != volumeVector_.end()) {
        int64_t duration = TimeUtils::GetCurSec() - (*it)->GetUint64Value("START_TIME");
        if (duration > 0 && duration > NEED_INCREASE_FREQUENCY) {
            (*it)->Add("DURATION", static_cast<uint64_t>(duration));
            mediaMonitorPolicy_.HandleVolumeToEventVector(*it);
            mediaMonitorPolicy_.WhetherToHiSysEvent();
        }
        volumeVector_.erase(it);
    }
}

void EventAggregate::HandleCaptureMutedStatusChange(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle capture muted status change");
    if (!bean->GetIntValue("MUTED")) {
        HandleCaptureMuted(bean);
    } else {
        uint64_t curruntTime = TimeUtils::GetCurSec();
        AddToCaptureMuteUsage(bean, curruntTime);
    }
}

void EventAggregate::HandleVolumeChange(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle volume change");
    mediaMonitorPolicy_.WriteEvent(bean->GetEventId(), bean);

    if (!bean->GetIntValue("ISOUTPUT")) {
        MEDIA_LOG_D("EventAggregate HandleVolumeChange is not playback");
        return;
    }
    auto isExist = [&bean](const std::shared_ptr<EventBean> &volumeBean) {
        if (bean->GetIntValue("STREAMID") == volumeBean->GetIntValue("STREAMID") &&
            bean->GetIntValue("DEVICETYPE") == volumeBean->GetIntValue("DEVICE_TYPE") &&
            bean->GetIntValue("SYSVOLUME") != volumeBean->GetIntValue("LEVEL")) {
            MEDIA_LOG_D("Find the existing volume vector");
            return true;
        }
        return false;
    };
    bool isInVolumeMap = false;
    auto it = std::find_if(volumeVector_.begin(), volumeVector_.end(), isExist);
    if (it != volumeVector_.end()) {
        (*it)->UpdateIntMap("LEVEL", bean->GetIntValue("SYSVOLUME"));
        if ((*it)->GetUint64Value("START_TIME") >= 0) {
            int64_t duration = TimeUtils::GetCurSec() - (*it)->GetUint64Value("START_TIME");
            if (duration > 0 && duration > NEED_INCREASE_FREQUENCY) {
                (*it)->Add("DURATION", static_cast<uint64_t>(duration));
                mediaMonitorPolicy_.HandleVolumeToEventVector(*it);
                mediaMonitorPolicy_.WhetherToHiSysEvent();
            }
        }
        isInVolumeMap = true;
    }
    if (!isInVolumeMap) {
        std::shared_ptr<EventBean> volumeBean = std::make_shared<EventBean>();
        volumeBean->Add("STREAMID", bean->GetIntValue("STREAMID"));
        volumeBean->Add("STREAM_TYPE", UNINITIALIZED);
        volumeBean->Add("DEVICE_TYPE", bean->GetIntValue("DEVICETYPE"));
        volumeBean->Add("LEVEL", bean->GetIntValue("SYSVOLUME"));
        volumeVector_.push_back(volumeBean);
    }
    // Record volume for stream state 2->5->2
    systemVol_ = bean->GetIntValue("SYSVOLUME");
}

void EventAggregate::HandlePipeChange(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle pipe change");
    AppExecFwk::BundleInfo bundleInfo = GetBundleInfoFromUid(bean->GetIntValue("CLIENT_UID"));
    bean->Add("APP_NAME", bundleInfo.name);
    mediaMonitorPolicy_.WriteEvent(bean->GetEventId(), bean);
}

void EventAggregate::HandleStreamExhaustedErrorEvent(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle stream exhausted error event");
    AppExecFwk::BundleInfo bundleInfo = GetBundleInfoFromUid(bean->GetIntValue("CLIENT_UID"));
    mediaMonitorPolicy_.HandleExhaustedToEventVector(bundleInfo.name);
    mediaMonitorPolicy_.WhetherToHiSysEvent();
}

void EventAggregate::HandleStreamCreateErrorEvent(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle stream create error event");
    AppExecFwk::BundleInfo bundleInfo = GetBundleInfoFromUid(bean->GetIntValue("CLIENT_UID"));
    bean->Add("APP_NAME", bundleInfo.name);
    mediaMonitorPolicy_.HandleCreateErrorToEventVector(bean);
    mediaMonitorPolicy_.WhetherToHiSysEvent();
}

void EventAggregate::HandleBackgroundSilentPlayback(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle background silent playback");
    AppExecFwk::BundleInfo bundleInfo = GetBundleInfoFromUid(bean->GetIntValue("CLIENT_UID"));
    bean->Add("APP_NAME", bundleInfo.name);
    bean->Add("APP_VERSION_CODE", static_cast<int32_t>(bundleInfo.versionCode));
    mediaMonitorPolicy_.HandleSilentPlaybackToEventVector(bean);
    mediaMonitorPolicy_.WhetherToHiSysEvent();
}

void EventAggregate::HandleUnderrunStatistic(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle underrun statistic");
    AppExecFwk::BundleInfo bundleInfo = GetBundleInfoFromUid(bean->GetIntValue("CLIENT_UID"));
    bean->Add("APP_NAME", bundleInfo.name);
    mediaMonitorPolicy_.HandleUnderrunToEventVector(bean);
    mediaMonitorPolicy_.WhetherToHiSysEvent();
}

void EventAggregate::HandleForceUseDevice(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle force use device");
    AppExecFwk::BundleInfo bundleInfo = GetBundleInfoFromUid(bean->GetIntValue("CLIENT_UID"));
    bean->Add("APP_NAME", bundleInfo.name);
    audioMemo_.UpdataRouteInfo(bean);
    mediaMonitorPolicy_.WriteEvent(bean->GetEventId(), bean);
}

void EventAggregate::HandleFocusMigrate(std::shared_ptr<EventBean> &bean)
{
    MEDIA_LOG_D("Handle focus use migrate");
    AppExecFwk::BundleInfo bundleInfo = GetBundleInfoFromUid(bean->GetIntValue("CLIENT_UID"));
    bean->Add("APP_NAME", bundleInfo.name);
    mediaMonitorPolicy_.WriteEvent(bean->GetEventId(), bean);
}

void EventAggregate::WriteInfo(int32_t fd, std::string &dumpString)
{
    if (fd != -1) {
        mediaMonitorPolicy_.WriteInfo(fd, dumpString);
    }
}

} // namespace MediaMonitor
} // namespace Media
} // namespace OHOS