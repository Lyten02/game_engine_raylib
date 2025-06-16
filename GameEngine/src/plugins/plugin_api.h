#pragma once

#include <functional>
#include <string>
#include <entt/entt.hpp>
#include "../systems/system.h"
#include "plugin_interface.h"

namespace GameEngine {

// Forward declarations
class PackageLoader;

// API that plugins use to interact with the engine
class PluginAPI {
public:
    PluginAPI(PackageLoader* loader) : packageLoader(loader) {}
    
    // Component registration
    using ComponentFactory = std::function<void(entt::registry&, entt::entity)>;
    virtual void registerComponent(const std::string& name, ComponentFactory factory);
    
    // System registration
    using SystemFactory = std::function<std::unique_ptr<ISystem>()>;
    virtual void registerSystem(const std::string& name, SystemFactory factory);
    
    // Logging
    virtual void log(const std::string& message);
    virtual void logError(const std::string& message);
    virtual void logWarning(const std::string& message);
    
    // Get version info
    virtual int getEngineAPIVersion() const { return PLUGIN_API_VERSION; }
    
private:
    PackageLoader* packageLoader;
};

} // namespace GameEngine