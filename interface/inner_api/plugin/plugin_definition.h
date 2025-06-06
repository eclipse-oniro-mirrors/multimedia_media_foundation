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

#ifndef HISTREAMER_PLUGINS_INTF_PLUGIN_DEFINITION_H
#define HISTREAMER_PLUGINS_INTF_PLUGIN_DEFINITION_H

#include <functional>
#include <string>
#include <memory>
#include "plugin_caps.h"
#include "plugin_base.h"
#include "plugin/plugin_buffer.h"

namespace OHOS {
namespace Media {
namespace Plugins {
/**
 * @brief Macro definition, creating the version information.
 *
 * @details The versioning is the process of assigning a unique version number to a unique state
 * of plugin interface. Within a given version number category (major, minor), these numbers are
 * usually assigned in ascending order and correspond to new developments in the plugin.
 *
 * Given a version number MAJOR.MINOR:
 *  - MAJOR: When you make incompatible API changes.
 *  - MINOR: When you add features in a backwards-compatible manner or do backwards-compatible bug fixes.
 */
#define MAKE_VERSION(MAJOR, MINOR) ((((MAJOR)&0xFFFF) << 16) | ((MINOR)&0xFFFF))

/// Plugin interface major number
#define PLUGIN_INTERFACE_VERSION_MAJOR (1)

/// Plugin interface minor number
#define PLUGIN_INTERFACE_VERSION_MINOR (0)

/// Plugin interface version
#define PLUGIN_INTERFACE_VERSION MAKE_VERSION(PLUGIN_INTERFACE_VERSION_MAJOR, PLUGIN_INTERFACE_VERSION_MINOR)

/**
 * @enum License Type.
 * an official permission or permit.
 *
 * @since 1.0
 * @version 1.0
 */
enum struct LicenseType : uint8_t {
    APACHE_V2, ///< The Apache License 2.0
    LGPL,      ///< The GNU Lesser General Public License
    GPL,       ///< The GNU General Public License
    CC0,       ///< The Creative Commons Zero v1.0 Universal
    VENDOR,    ///< Offered by Vendor
    UNKNOWN,   ///< Unknown License
};

/**
 * @brief Definition of plugin packaging information.
 *
 * @since 1.0
 * @version 1.0
 */
struct PackageDef {
    uint32_t pkgVersion;    ///< Package information version, which indicates the latest plug-in interface version
                            ///< used by the plugin in the package. The default value is PLUGIN_INTERFACE_VERSION.

    std::string name;   ///< Package name. The plugin framework registers the plugin using this name.
                        ///< If the plugins are packaged as a dynamic library, the name of library
                        ///< must be in the format of "libplugin_<name>.so".

    LicenseType
        licenseType;    ///< The License information of the plugin in the package.
                        ///< The different plugins must be the same.
                        ///< The plugin framework processing in the plugin running state based on different license.
};

/**
 * @brief Data source operation interface.
 *
 * @since 1.0
 * @version 1.0
 */
struct DataSource {
    /// Destructor
    virtual ~DataSource() = default;

    /**
     * @brief Read data from data source.
     *
     * @param offset    Offset of read position
     * @param buffer    Storage of the read data
     * @param expectedLen   Expected data size to be read
     * @return  Execution status return
     *  @retval OK: Plugin ReadAt succeeded.
     *  @retval ERROR_NOT_ENOUGH_DATA: Data not enough
     *  @retval END_OF_STREAM: End of stream
     */
    virtual Status ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen) = 0;

    /**
     * @brief Get data source size.
     *
     * @param size data source size.
     * @return  Execution status return.
     *  @retval OK: Plugin GetSize succeeded.
     */
    virtual Status GetSize(uint64_t& size) = 0;

    /**
     * @brief Indicates that the current data source seekable or not.
     *
     * The function is valid only after INITIALIZED state.
     *
     * @return  Seekable status
     */
    virtual Seekable GetSeekable() = 0;

    virtual int32_t GetStreamID() = 0;

    virtual bool IsDash() = 0;
};

/// Plugin create function. All plugins must implement this function.
template <typename T>
using PluginCreatorFunc = std::function<std::shared_ptr<T>(const std::string& name)>;

/// Sniff function
using PluginSnifferFunc = int (*)(const std::string& name, std::shared_ptr<DataSource> dataSource);

/**
 * @brief Describes the basic information about the plugin.
 *
 * @since 1.0
 * @version 1.0
 */
struct PluginDefBase {
    uint32_t apiVersion{0}; ///< Versions of different plugins. Different types of plugin have their own versions.

    PluginType pluginType = PluginType::INVALID_TYPE; ///< Describe the plugin type, e.g. 'source', 'codec'.

    std::string name;   ///< Indicates the name of a plugin. The name of the same type plugins must be unique.
                        ///< Plugins with the same name may fail to be registered.

    std::string description; ///< Detailed description of the plugin.

    uint32_t rank{0};  ///< Plugin score. The plugin with a high score may be preferred. You can evaluate the
                    ///< plugin score in terms of performance, version support, and license. Range: 0 to 100.

    PluginDefBase()
    {
        pluginType = PluginType::INVALID_TYPE;
    }

    virtual ~PluginDefBase() {}

    virtual void AddInCaps(Capability& capability)
    {
        inCaps.emplace_back(capability);
    }

    virtual void AddOutCaps(Capability& capability)
    {
        outCaps.emplace_back(capability);
    }

    virtual void AddExtensions(std::vector<std::string> ex)
    {
        auto iter = ex.begin();
        while (iter != ex.end()) {
            extensions.emplace_back(*iter);
            iter++;
        }
    }

    virtual std::vector<std::string> GetExtensions() const
    {
        return extensions;
    }

    virtual CapabilitySet GetInCaps() const
    {
        return inCaps;
    }

    virtual CapabilitySet GetOutCaps() const
    {
        return outCaps;
    }

    virtual PluginCreatorFunc<PluginBase> GetCreator() const
    {
        return creator;
    }

    virtual void SetCreator(PluginCreatorFunc<PluginBase> creatorFunc)
    {
        creator = creatorFunc;
    }

    virtual PluginSnifferFunc GetSniffer() const
    {
        return sniffer;
    }

    virtual void SetSniffer(PluginSnifferFunc sniffFunc)
    {
        sniffer = sniffFunc;
    }

private:
    std::vector<std::string> extensions;      ///< File extensions
    CapabilitySet inCaps;                     ///< Plug-in input capability, For details, @see Capability.
    CapabilitySet outCaps;                    ///< Plug-in output capability, For details, @see Capability.
    PluginCreatorFunc<PluginBase> creator {nullptr}; ///< plugin create function.
    PluginSnifferFunc sniffer {nullptr};         ///< plugin sniff function.
};

/**
 * @brief The plugin registration interface.
 * The plugin framework will provide the implementation.
 * Developers only need to invoke the API to register the plugin.
 *
 * @since 1.0
 * @version 1.0
 */
struct Register {
    virtual ~Register() = default;
    /**
     * @brief Register the plugin.
     *
     * @param def   Basic information about the plugin
     * @return  Registration status return
     *  @retval OK: The plugin is registered succeed.
     *  @retval ERROR_PLUGIN_ALREADY_EXISTS: The plugin already exists in plugin registered.
     *  @retval ERROR_INCOMPATIBLE_VERSION: Incompatible version during plugin registration.
     */
    virtual Status AddPlugin(const PluginDefBase& def) = 0;
};

/**
 * @brief The package registration interface.
 * The plugin framework will provide the implementation and auto invoke the API to
 * finish the package registration when plugin framework first time be initialized.
 *
 * @since 1.0
 * @version 1.0
 */
struct PackageRegister : Register {
    ~PackageRegister() override = default;

    /**
     * @brief Register the package.
     * During package registration, all plugins in the package are automatically registered.
     *
     * @param def   plugin packaging information.
     * @return  Registration status return
     *  @retval OK: The package is registered succeed without any errors.
     *  @retval ERROR_PLUGIN_ALREADY_EXISTS: The package or plugins already exists.
     *  @retval ERROR_INCOMPATIBLE_VERSION: Incompatible plugin interface version or api version.
     */
    virtual Status AddPackage(const PackageDef& def) = 0;
};

/// Plugin registration function, all plugins must be implemented.
using RegisterFunc = Status (*)(const std::shared_ptr<PackageRegister>& reg);

/// Plugin deregister function, all plugins must be implemented.
using UnregisterFunc = void (*)();

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#else
#define PLUGIN_EXPORT
#endif
#endif

/// Macro definition, string concatenation
#define PLUGIN_PASTE_ARGS(str1, str2) str1##str2

/// Macro definition, string concatenation
#define PLUGIN_PASTE(str1, str2) PLUGIN_PASTE_ARGS(str1, str2)

/// Macro definition, stringify
#define PLUGIN_STRINGIFY_ARG(str) #str

/// Macro definition, stringify
#define PLUGIN_STRINGIFY(str) PLUGIN_STRINGIFY_ARG(str)

/**
 * @brief Macro definition, Defines basic plugin information.
 * Which is invoked during plugin package registration. All plugin packages must be implemented.
 *
 * @param name              Package name. For details, @see PackageDef::name
 * @param license           Package License, For details, @see PackageDef::licenseType
 * @param registerFunc      Plugin registration function, MUST NOT be NULL.
 * @param unregisterFunc    Plugin deregister function，MUST NOT be NULL.
 */
#define PLUGIN_DEFINITION(name, license, registerFunc, unregisterFunc)                                          \
    PLUGIN_EXPORT OHOS::Media::Status PLUGIN_PASTE(register_, name)(                                            \
        const std::shared_ptr<OHOS::Media::Plugins::PackageRegister>& pkgReg)                                   \
    {                                                                                                           \
        pkgReg->AddPackage({PLUGIN_INTERFACE_VERSION, PLUGIN_STRINGIFY(name), license});                        \
        std::shared_ptr<OHOS::Media::Plugins::Register> pluginReg = pkgReg;                                     \
        return registerFunc(pluginReg);                                                                         \
    }                                                                                                           \
    PLUGIN_EXPORT void PLUGIN_PASTE(unregister_, name)()                                                        \
    {                                                                                                           \
        unregisterFunc();                                                                                       \
    }
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_PLUGINS_INTF_PLUGIN_DEFINITION_H
