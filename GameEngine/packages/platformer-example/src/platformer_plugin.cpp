#include "platformer_game_logic.h"
#include <plugin_api.h>
#include <memory>
#include <spdlog/spdlog.h>

using namespace GameEngine;
using namespace PlatformerExample;

// Factory function for creating platformer game logic
std::unique_ptr<IGameLogic> createPlatformerGameLogic() {
    return std::make_unique<PlatformerGameLogic>();
}

// Plugin initialization function
extern "C" {
    
void initializePlugin(IPluginManager* manager) {
    if (!manager) {
        spdlog::error("[PlatformerPlugin] Invalid plugin manager");
        return;
    }
    
    spdlog::info("[PlatformerPlugin] Initializing platformer example plugin");
    
    // Register platformer game logic factory
    manager->registerGameLogicFactory("PlatformerGameLogic", createPlatformerGameLogic);
    spdlog::info("[PlatformerPlugin] Registered PlatformerGameLogic");
}

const char* getPluginName() {
    return "platformer-example";
}

const char* getPluginVersion() {
    return "1.0.0";
}

const char* getPluginDescription() {
    return "Example platformer game logic plugin";
}

} // extern "C"