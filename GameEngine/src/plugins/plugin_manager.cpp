#include "plugin_manager.h"
#include "plugin_api.h"
#include <spdlog/spdlog.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace GameEngine {

PluginManager::PluginManager(PackageLoader* loader) 
    : packageLoader(loader) {
}

PluginManager::~PluginManager() {
    unloadAllPlugins();
}

bool PluginManager::loadPlugin(const std::filesystem::path& pluginPath) {
    // Check if file exists
    if (!std::filesystem::exists(pluginPath)) {
        lastError = "Plugin file not found: " + pluginPath.string();
        spdlog::error("[PluginManager] {}", lastError);
        return false;
    }
    
    // Load the library
    LibraryHandle handle = loadLibrary(pluginPath);
    if (!handle) {
        return false;
    }
    
    // Validate plugin
    if (!validatePlugin(handle)) {
        unloadLibrary(handle);
        return false;
    }
    
    // Get plugin creation functions
    auto createFunc = reinterpret_cast<CreatePluginFunc>(getSymbol(handle, "createPlugin"));
    auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(getSymbol(handle, "destroyPlugin"));
    
    if (!createFunc || !destroyFunc) {
        lastError = "Plugin missing required export functions";
        spdlog::error("[PluginManager] {}", lastError);
        unloadLibrary(handle);
        return false;
    }
    
    // Create plugin instance
    IPlugin* pluginInstance = createFunc();
    if (!pluginInstance) {
        lastError = "Failed to create plugin instance";
        spdlog::error("[PluginManager] {}", lastError);
        unloadLibrary(handle);
        return false;
    }
    
    // Get plugin info
    PluginInfo info = pluginInstance->getInfo();
    
    // Check if plugin already loaded
    if (loadedPlugins.find(info.name) != loadedPlugins.end()) {
        lastError = "Plugin already loaded: " + info.name;
        spdlog::warn("[PluginManager] {}", lastError);
        destroyFunc(pluginInstance);
        unloadLibrary(handle);
        return false;
    }
    
    // Create plugin API
    auto api = std::make_unique<PluginAPI>(packageLoader);
    
    // Initialize plugin
    if (!pluginInstance->onLoad(api.get())) {
        lastError = "Plugin initialization failed";
        spdlog::error("[PluginManager] {}", lastError);
        destroyFunc(pluginInstance);
        unloadLibrary(handle);
        return false;
    }
    
    // Store loaded plugin
    auto loadedPlugin = std::make_unique<LoadedPlugin>();
    loadedPlugin->path = pluginPath;
    loadedPlugin->handle = handle;
    loadedPlugin->instance.reset(pluginInstance);
    loadedPlugin->api = std::move(api);
    loadedPlugin->info = info;
    
    loadedPlugins[info.name] = std::move(loadedPlugin);
    
    spdlog::info("[PluginManager] Loaded plugin: {} v{}", info.name, info.version);
    return true;
}

bool PluginManager::unloadPlugin(const std::string& pluginName) {
    auto it = loadedPlugins.find(pluginName);
    if (it == loadedPlugins.end()) {
        lastError = "Plugin not loaded: " + pluginName;
        return false;
    }
    
    auto& plugin = it->second;
    
    // Call unload callback
    plugin->instance->onUnload();
    
    // Get destroy function before unloading library
    auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(
        getSymbol(plugin->handle, "destroyPlugin")
    );
    
    // Destroy plugin instance
    if (destroyFunc) {
        destroyFunc(plugin->instance.release());
    }
    
    // Unload library
    unloadLibrary(plugin->handle);
    
    // Remove from loaded plugins
    loadedPlugins.erase(it);
    
    spdlog::info("[PluginManager] Unloaded plugin: {}", pluginName);
    return true;
}

void PluginManager::unloadAllPlugins() {
    // Copy plugin names to avoid iterator invalidation
    std::vector<std::string> pluginNames;
    for (const auto& [name, plugin] : loadedPlugins) {
        pluginNames.push_back(name);
    }
    
    // Unload each plugin
    for (const auto& name : pluginNames) {
        unloadPlugin(name);
    }
}

bool PluginManager::isPluginLoaded(const std::string& pluginName) const {
    return loadedPlugins.find(pluginName) != loadedPlugins.end();
}

std::vector<std::string> PluginManager::getLoadedPlugins() const {
    std::vector<std::string> result;
    for (const auto& [name, plugin] : loadedPlugins) {
        result.push_back(name);
    }
    return result;
}

const PluginInfo* PluginManager::getPluginInfo(const std::string& pluginName) const {
    auto it = loadedPlugins.find(pluginName);
    if (it != loadedPlugins.end()) {
        return &it->second->info;
    }
    return nullptr;
}

LibraryHandle PluginManager::loadLibrary(const std::filesystem::path& path) {
#ifdef _WIN32
    LibraryHandle handle = LoadLibraryA(path.string().c_str());
    if (!handle) {
        lastError = "Failed to load library: " + std::to_string(GetLastError());
        spdlog::error("[PluginManager] {}", lastError);
    }
    return handle;
#else
    LibraryHandle handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        lastError = "Failed to load library: " + std::string(dlerror());
        spdlog::error("[PluginManager] {}", lastError);
    }
    return handle;
#endif
}

void PluginManager::unloadLibrary(LibraryHandle handle) {
    if (!handle) return;
    
#ifdef _WIN32
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
}

void* PluginManager::getSymbol(LibraryHandle handle, const std::string& name) {
    if (!handle) return nullptr;
    
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(handle, name.c_str()));
#else
    return dlsym(handle, name.c_str());
#endif
}

bool PluginManager::validatePlugin(LibraryHandle handle) {
    // Check API version
    auto getVersionFunc = reinterpret_cast<GetPluginAPIVersionFunc>(
        getSymbol(handle, "getPluginAPIVersion")
    );
    
    if (!getVersionFunc) {
        lastError = "Plugin missing getPluginAPIVersion export";
        spdlog::error("[PluginManager] {}", lastError);
        return false;
    }
    
    int pluginApiVersion = getVersionFunc();
    if (pluginApiVersion != PLUGIN_API_VERSION) {
        lastError = "Plugin API version mismatch. Expected: " + 
                   std::to_string(PLUGIN_API_VERSION) + 
                   ", Got: " + std::to_string(pluginApiVersion);
        spdlog::error("[PluginManager] {}", lastError);
        return false;
    }
    
    return true;
}

} // namespace GameEngine