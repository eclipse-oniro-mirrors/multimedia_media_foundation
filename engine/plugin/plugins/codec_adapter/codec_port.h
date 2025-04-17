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
#if defined(VIDEO_SUPPORT)

#ifndef HISTREAMER_PLUGIN_CODEC_PORT_H
#define HISTREAMER_PLUGIN_CODEC_PORT_H
#include "v3_0/icodec_component.h"
#include "v3_0/codec_types.h"
#include "foundation/osal/thread/condition_variable.h"
#include "foundation/osal/thread/mutex.h"
#include "plugin/common/plugin_types.h"
#include "plugin/common/plugin_meta.h"
#include "OMX_Component.h"

namespace OHOS {
namespace Media {
namespace Plugin {
namespace CodecAdapter {
namespace CodecHDI = OHOS::HDI::Codec::V3_0;
struct PortInfo {
    uint32_t bufferCount{};
    uint32_t bufferSize{};
    bool enabled{false};
};

class CodecPort {
public:
    CodecPort(sptr<CodecHDI::ICodecComponent> component, uint32_t portIndex, const CodecHDI::CompVerInfo& verInfo);
    ~CodecPort() = default;
    Status Config(Meta& meta);
    Status QueryParam(PortInfo& portInfo);
private:
    sptr<CodecHDI::ICodecComponent> codecComp_ {nullptr};
    OMX_PARAM_PORTDEFINITIONTYPE portDef_ {};
    CodecHDI::CompVerInfo verInfo_ {};
};
} // namespace CodecAdapter
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_PLUGIN_CODEC_PORT_H
#endif