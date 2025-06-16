#include "platformer_game_logic.h"
#include <plugin_api.h>
#include <scripting/game_logic_manager.h>
#include <memory>
#include <spdlog/spdlog.h>

using namespace PlatformerExample;

// Factory function for creating platformer game logic
std::unique_ptr<IGameLogic> createPlatformerGameLogic() {
    return std::make_unique<PlatformerGameLogic>();
}

// Plugin initialization function
extern "C" {
    
bool platformer_plugin_init(PluginAPI* api) {
    if (!api) {
        return false;
    }
    
    spdlog::info("[PlatformerPlugin] Initializing platformer example plugin");
    
    // Get game logic manager from API
    auto* gameLogicManager = api->getGameLogicManager();
    if (!gameLogicManager) {
        spdlog::error("[PlatformerPlugin] Failed to get GameLogicManager from API");
        return false;
    }
    
    // Register platformer game logic
    gameLogicManager->registerLogicFactory("PlatformerGameLogic", createPlatformerGameLogic);
    spdlog::info("[PlatformerPlugin] Registered PlatformerGameLogic");
    
    return true;
}

void platformer_plugin_shutdown(PluginAPI* api) {
    spdlog::info("[PlatformerPlugin] Shutting down platformer example plugin");
}

} // extern "C"