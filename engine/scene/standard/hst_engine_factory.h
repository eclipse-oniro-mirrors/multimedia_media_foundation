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

#ifndef HISTREAMER_HST_ENGINE_FACTORY_H
#define HISTREAMER_HST_ENGINE_FACTORY_H

#include "i_engine_factory.h"

namespace OHOS {
namespace Media {
class HstEngineFactory : public IEngineFactory {
public:
    HstEngineFactory() = default;
    ~HstEngineFactory() override = default;

    int32_t Score(Scene scene, const std::string& uri) override;
#ifdef SUPPORT_PLAYER
    std::unique_ptr<IPlayerEngine> CreatePlayerEngine(
        int32_t appUid = 0, int32_t appPid = 0, uint32_t appTokenId = 0) override;
#endif
#ifdef SUPPORT_RECORDER
    std::unique_ptr<IRecorderEngine> CreateRecorderEngine(int32_t appUid, int32_t appPid,
        uint32_t appTokenId, uint64_t appFullTokenId) override;
#endif
#ifdef SUPPORT_METADATA
    std::unique_ptr<IAVMetadataHelperEngine> CreateAVMetadataHelperEngine() override;
#endif
#ifdef SUPPORT_CODEC
    std::unique_ptr<IAVCodecEngine> CreateAVCodecEngine() override;
    std::unique_ptr<IAVCodecListEngine> CreateAVCodecListEngine() override;
#endif
#ifdef SUPPORT_MUXER
    std::unique_ptr<IAVMuxerEngine> CreateAVMuxerEngine() override;
#endif
};
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_HST_ENGINE_FACTORY_H
