#pragma once

#include <string>

namespace GameEngine {
// Forward declarations
class PluginAPI;

// Plugin API version for compatibility checking
#define PLUGIN_API_VERSION 1

// Plugin metadata
struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    int apiVersion;
};

// Base interface that all plugins must implement
class IPlugin {
public:
    virtual ~IPlugin() = default;

    // Called when plugin is loaded
    virtual bool onLoad(PluginAPI* api) = 0;

    // Called when plugin is unloaded
    virtual void onUnload() = 0;

    // Get plugin information
    virtual PluginInfo getInfo() const = 0;
};

// Export functions that plugins must implement
extern "C" {
    // Create plugin instance
    typedef IPlugin* (*CreatePluginFunc)();

    // Destroy plugin instance
    typedef void (*DestroyPluginFunc)(IPlugin*);

    // Get API version
    typedef int (*GetPluginAPIVersionFunc)();
}

// Macros for plugin implementation
#define PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))

#define IMPLEMENT_PLUGIN(PluginClass) \
    PLUGIN_EXPORT GameEngine::IPlugin* createPlugin() { \
        return new PluginClass(); \
    } \
    PLUGIN_EXPORT void destroyPlugin(GameEngine::IPlugin* plugin) { \
        delete plugin; \
    } \
    PLUGIN_EXPORT int getPluginAPIVersion() { \
        return PLUGIN_API_VERSION; \
    }

} // namespace GameEngine
