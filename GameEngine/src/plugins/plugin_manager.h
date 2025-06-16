#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <filesystem>
#include "plugin_interface.h"

namespace GameEngine {

// Forward declarations
class PluginAPI;
class PackageLoader;

// Platform-specific library handle
#ifdef _WIN32
    #include <windows.h>
    using LibraryHandle = HMODULE;
#else
    using LibraryHandle = void*;
#endif

// Loaded plugin information
struct LoadedPlugin {
    std::filesystem::path path;
    LibraryHandle handle;
    std::unique_ptr<IPlugin> instance;
    std::unique_ptr<PluginAPI> api;
    PluginInfo info;
};

class PluginManager {
public:
    PluginManager(PackageLoader* loader);
    ~PluginManager();
    
    // Plugin loading/unloading
    bool loadPlugin(const std::filesystem::path& pluginPath);
    bool unloadPlugin(const std::string& pluginName);
    void unloadAllPlugins();
    
    // Plugin queries
    bool isPluginLoaded(const std::string& pluginName) const;
    std::vector<std::string> getLoadedPlugins() const;
    const PluginInfo* getPluginInfo(const std::string& pluginName) const;
    
    // Error handling
    std::string getLastError() const { return lastError; }
    
private:
    // Platform-specific library loading
    LibraryHandle loadLibrary(const std::filesystem::path& path);
    void unloadLibrary(LibraryHandle handle);
    void* getSymbol(LibraryHandle handle, const std::string& name);
    
    // Plugin validation
    bool validatePlugin(LibraryHandle handle);
    
    std::unordered_map<std::string, std::unique_ptr<LoadedPlugin>> loadedPlugins;
    PackageLoader* packageLoader;
    std::string lastError;
};

} // namespace GameEngine