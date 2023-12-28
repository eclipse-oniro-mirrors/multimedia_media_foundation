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

#ifndef HISTREAMER_PLUGIN_REGISTER_H
#define HISTREAMER_PLUGIN_REGISTER_H

#include <functional>
#include <map>
#include <set>
#include <utility>
#include "meta/any.h"
#include "plugin/plugin_loader.h"
#include "plugin/plugin_info.h"
#include "plugin/generic_plugin.h"

namespace OHOS {
namespace Media {
namespace Plugins {
struct DataSource;
using DemuxerPluginSnifferFunc = int (*)(const std::string& name, std::shared_ptr<DataSource> dataSource);
struct PluginRegInfo {
    std::shared_ptr<PackageDef> packageDef;
    std::shared_ptr<PluginInfo> info;
    PluginCreatorFunc<PluginBase> creator;
    DemuxerPluginSnifferFunc sniffer;
    std::shared_ptr<PluginLoader> loader;
};

class PluginRegister {
public:
    PluginRegister() = default;
    PluginRegister(const PluginRegister&) = delete;
    PluginRegister operator=(const PluginRegister&) = delete;
    ~PluginRegister();

    std::vector<std::string> ListPlugins(PluginType type, CodecMode preferredCodecMode = CodecMode::HARDWARE);
    int GetAllRegisteredPluginCount();

    std::shared_ptr<PluginRegInfo> GetPluginRegInfo(PluginType type, const std::string& name);

    void RegisterPlugins();

    void RegisterGenericPlugin(const GenericPluginDef& pluginDef);

    void RegisterGenericPlugins(const std::vector<GenericPluginDef>& vecPluginDef);
private:
    void RegisterStaticPlugins();
    void RegisterDynamicPlugins();
    void RegisterPluginsFromPath(const char* libDirPath);
    void UnregisterAllPlugins();
    void EraseRegisteredPluginsByLoader(const std::shared_ptr<PluginLoader>& loader);

private:
    using REGISTERED_TABLE = std::map<PluginType, std::map<std::string, std::shared_ptr<PluginRegInfo>>>;

    struct RegisterData {
        std::map<PluginType, std::vector<std::string>> registerNames;
        REGISTERED_TABLE registerTable;
        bool IsPluginExist(PluginType type, const std::string& name);
    };

    struct RegisterImpl : PackageRegister {
        explicit RegisterImpl(std::shared_ptr<RegisterData> data, std::shared_ptr<PluginLoader> loader = nullptr)
            : pluginLoader(std::move(loader)), registerData(std::move(data)) {}

        ~RegisterImpl() override = default;

        Status AddPlugin(const PluginDefBase& def) override;

        Status AddPackage(const PackageDef& def) override;

        Status SetPackageDef(const PackageDef& def);

        void UpdateRegisterTableAndRegisterNames(const PluginDefBase& def);

        void SetPluginInfo(std::shared_ptr<PluginInfo>& info, const PluginDefBase& def);

        Status InitSourceInfo(std::shared_ptr<PluginRegInfo>& reg, const PluginDefBase& def);

        Status InitDemuxerInfo(std::shared_ptr<PluginRegInfo>& reg, const PluginDefBase& def);

        Status InitMuxerInfo(std::shared_ptr<PluginRegInfo>& reg, const PluginDefBase& def);

        Status InitCodecInfo(std::shared_ptr<PluginRegInfo>& reg, const PluginDefBase& def);

        Status InitAudioSinkInfo(std::shared_ptr<PluginRegInfo>& reg, const PluginDefBase& def);

        Status InitVideoSinkInfo(std::shared_ptr<PluginRegInfo>& reg, const PluginDefBase& def);

        Status InitOutputSinkInfo(std::shared_ptr<PluginRegInfo>& reg, const PluginDefBase& def);

        Status InitGenericPlugin(std::shared_ptr<PluginRegInfo>& reg, const PluginDefBase& def);

        bool Verification(const PluginDefBase& definition);

        bool VersionMatched(const PluginDefBase& definition);

        bool MoreAcceptable(std::shared_ptr<PluginRegInfo>& regInfo, const PluginDefBase& def);

        std::shared_ptr<PluginLoader> pluginLoader;
        std::shared_ptr<RegisterData> registerData;
        std::shared_ptr<PackageDef> packageDef {nullptr};
    };
    void DeletePlugin(std::map<std::string, std::shared_ptr<PluginRegInfo>>& plugins,
        std::map<std::string, std::shared_ptr<PluginRegInfo>>::iterator& info);
    std::shared_ptr<RegisterData> registerData_ = std::make_shared<RegisterData>();
    std::vector<std::shared_ptr<PluginLoader>> registeredLoaders_;
    std::shared_ptr<RegisterImpl> staticPluginRegister_ = std::make_shared<RegisterImpl>(registerData_);
public:
    std::shared_ptr<RegisterImpl> GetStaticPluginRegister()
    {
        return staticPluginRegister_;
    }
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_PLUGIN_REGISTER_H
