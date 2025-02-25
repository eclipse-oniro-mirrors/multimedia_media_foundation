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

#ifndef HISTREAMER_NETWORK_CLIENT_H
#define HISTREAMER_NETWORK_CLIENT_H

#include <string>
#include <map>
#include "network/network_typs.h"
#include "plugin/plugin_event.h"
#include "meta/media_types.h"
#include "common/status.h"
#include "common/media_source.h"
#include "common/avsharedmemorypool.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class NetworkClient {
public:
    virtual ~NetworkClient() = default;
    virtual Status Init() = 0;
    virtual Status Open(const std::string& url, const std::map<std::string, std::string>& httpHeader,
        int32_t timeoutMs) = 0;
    virtual Status RequestData(long startPos, int len, const RequestInfo& sourceInfo,
        HandleResponseCbFunc completedCb) = 0;
    virtual Status Close(bool isAsync) = 0;
    virtual Status Deinit() = 0;
    virtual Status GetIp(std::string &ip);
    static std::shared_ptr<NetworkClient> GetInstance(RxHeader headCallback, RxBody bodyCallback, void *userParam);
    virtual void SetAppUid(int32_t appUid);
    static std::shared_ptr<NetworkClient> GetAppInstance(RxHeader headCallback, RxBody bodyCallback, void *userParam);
    virtual void SetLoader(std::shared_ptr<IMediaSourceLoader> sourceLoader) {}
    virtual int32_t RespondHeader(int64_t uuid, const std::map<std::string, std::string>& httpHeader,
                    std::string redirctUrl)
    {
        return 0;
    }
    virtual int32_t RespondData(int64_t uuid, int64_t offset, const std::shared_ptr<AVSharedMemory> memory)
    {
        return 0;
    }
    virtual int32_t FinishLoading(int64_t uuid, LoadingRequestError state)
    {
        return 0;
    }
    virtual void SetUuid(int64_t uuid) {}

    virtual std::string GetRedirectUrl()
    {
        return "";
    }
};
} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_NETWORK_CLIENT_H