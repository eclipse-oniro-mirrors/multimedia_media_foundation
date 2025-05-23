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

#include "parameters.h"
#include "media_monitor_manager.h"
#include "event_bean.h"
#include "monitor_utils.h"
#include "monitor_error.h"
#include "media_monitor_manager_unit_test.h"

using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace Media {
namespace MediaMonitor {

void MediaMonitorManagerUnitTest::SetUpTestCase(void) {}
void MediaMonitorManagerUnitTest::TearDownTestCase(void) {}
void MediaMonitorManagerUnitTest::SetUp(void) {}
void MediaMonitorManagerUnitTest::TearDown(void) {}


HWTEST(MediaMonitorManagerUnitTest, Monitor_Manager_WriteLogMsg_001, TestSize.Level0)
{
    int32_t isHasMic = 1;
    int32_t isConnected = 1;
    int32_t deviceType = 2;
    uint64_t duration = TimeUtils::GetCurSec();
    std::shared_ptr<EventBean> bean = std::make_shared<EventBean>(
        AUDIO, HEADSET_CHANGE, BEHAVIOR_EVENT);
        bean->Add("HASMIC", isHasMic);
        bean->Add("ISCONNECT", isConnected);
        bean->Add("DEVICETYPE", deviceType);
        bean->Add("START_TIME", duration);
    MediaMonitorManager::GetInstance().WriteLogMsg(bean);

    EXPECT_EQ(bean->GetIntValue("HASMIC"), isHasMic);
    EXPECT_EQ(bean->GetIntValue("ISCONNECT"), isConnected);
    EXPECT_EQ(bean->GetIntValue("DEVICETYPE"), deviceType);
    EXPECT_EQ(bean->GetUint64Value("START_TIME"), duration);
}

HWTEST(MediaMonitorManagerUnitTest, Monitor_Manager_WriteLogMsg_002, TestSize.Level0)
{
    ModuleId moduleId = AUDIO;
    EventId eventId = LOAD_CONFIG_ERROR;
    EventType eventType = FAULT_EVENT;

    int32_t isHasMic = 1;
    int32_t isConnected = 1;
    int32_t deviceType = 2;
    std::shared_ptr<EventBean> bean = std::make_shared<EventBean>(
        AUDIO, eventId, eventType);
        bean->Add("HASMIC", isHasMic);
        bean->Add("ISCONNECT", isConnected);
        bean->Add("DEVICETYPE", deviceType);
    MediaMonitorManager::GetInstance().WriteLogMsg(bean);

    EXPECT_EQ(bean->GetModuleId(), moduleId);
    EXPECT_EQ(bean->GetEventId(), eventId);
    EXPECT_EQ(bean->GetEventType(), eventType);
}

HWTEST(MediaMonitorManagerUnitTest, Monitor_Manager_WriteLogMsg_003, TestSize.Level0)
{
    ModuleId moduleId = AUDIO;
    EventId eventId = LOAD_CONFIG_ERROR;
    EventType eventType = FAULT_EVENT;

    int32_t isHasMic1 = 1;
    int32_t isConnected1 = 1;
    int32_t deviceType1 = 2;

    int32_t isHasMic2 = 1;
    int32_t isConnected2 = 1;
    int32_t deviceType2 = 2;
    std::shared_ptr<EventBean> bean = std::make_shared<EventBean>(
        moduleId, eventId, eventType);
    bean->Add("HASMIC", isHasMic1);
    bean->Add("ISCONNECT", isConnected1);
    bean->Add("DEVICETYPE", deviceType1);

    bean->UpdateIntMap("HASMIC", isHasMic2);
    bean->UpdateIntMap("ISCONNECT", isConnected2);
    bean->UpdateIntMap("DEVICETYPE", deviceType2);

    MediaMonitorManager::GetInstance().WriteLogMsg(bean);

    EXPECT_EQ(bean->GetIntValue("HASMIC"), isHasMic2);
    EXPECT_EQ(bean->GetIntValue("ISCONNECT"), isConnected2);
    EXPECT_EQ(bean->GetIntValue("DEVICETYPE"), deviceType2);
}

HWTEST(MediaMonitorManagerUnitTest, Monitor_Manager_SetMediaParams_001, TestSize.Level0)
{
    std::string version = OHOS::system::GetParameter("const.logsystem.versiontype", COMMERCIAL_VERSION);
    size_t size = 0;
    std::vector<std::pair<std::string, std::string>> kvpairs;
    kvpairs.push_back({"BETA", "true"});
    size = kvpairs.size();
    int32_t ret = MediaMonitorManager::GetInstance().SetMediaParameters(kvpairs);
    if (version != BETA_VERSION) {
        EXPECT_EQ(ret, ERROR);
        return;
    }
    EXPECT_EQ(kvpairs.size(), size);
}

HWTEST(MediaMonitorManagerUnitTest, Monitor_Manager_SetMediaParams_002, TestSize.Level0)
{
    std::string version = OHOS::system::GetParameter("const.logsystem.versiontype", COMMERCIAL_VERSION);
    size_t size = 0;
    std::vector<std::pair<std::string, std::string>> kvpairs;
    kvpairs.push_back({"BETA", "false"});
    size = kvpairs.size();
    int32_t ret = MediaMonitorManager::GetInstance().SetMediaParameters(kvpairs);
    if (version != BETA_VERSION) {
        EXPECT_EQ(ret, ERROR);
        return;
    }
    EXPECT_EQ(kvpairs.size(), size);
}

HWTEST(MediaMonitorManagerUnitTest, Monitor_Manager_GetMediaParams_001, TestSize.Level0)
{
    std::string version = OHOS::system::GetParameter("const.logsystem.versiontype", COMMERCIAL_VERSION);
    size_t size = 1;
    std::vector<std::string> subKeys;
    subKeys.emplace_back("BETA");
    std::vector<std::pair<std::string, std::string>> result;
    int32_t ret = MediaMonitorManager::GetInstance().GetMediaParameters(subKeys, result);
    if (version != BETA_VERSION) {
        EXPECT_EQ(ret, ERROR);
        return;
    }
    EXPECT_EQ(result.size(), size);
}

HWTEST(MediaMonitorManagerUnitTest, Monitor_Manager_WriteAudioBuffer_001, TestSize.Level0)
{
    std::string filename = "/data/log/audiodump/unit_test_48000_1_1.pcm";
    uint8_t *buffer = nullptr;
    MediaMonitorManager::GetInstance().WriteAudioBuffer(filename, static_cast<void *>(buffer), 0);
    EXPECT_EQ(buffer, nullptr);
}

} // namespace MediaMonitor
} // namespace Media
} // namespace OHOS