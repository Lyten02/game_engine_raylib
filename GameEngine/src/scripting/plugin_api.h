#pragma once

#include "game_logic_interface.h"
#include <functional>
#include <memory>
#include <string>

namespace GameEngine {

// Plugin manager interface
class IPluginManager {
public:
    virtual ~IPluginManager() = default;
    virtual void registerGameLogicFactory(const std::string& name, std::function<std::unique_ptr<IGameLogic>()> factory) = 0;
};

// Plugin initialization function type
using PluginInitFunction = void(*)(IPluginManager* manager);

// Plugin info structure
struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
};

// Plugin interface
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual void initialize(IPluginManager* manager) = 0;
    virtual void shutdown() = 0;
    virtual PluginInfo getInfo() const = 0;
};

} // namespace GameEngine

// C-style exports for plugin loading
extern "C" {
    // Every plugin must export this function
    void initializePlugin(GameEngine::IPluginManager* manager);
    
    // Optional: plugin info functions (return individual components to avoid C linkage issues)
    const char* getPluginName();
    const char* getPluginVersion();
    const char* getPluginDescription();
}