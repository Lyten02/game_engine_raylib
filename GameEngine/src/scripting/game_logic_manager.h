#pragma once

#include "game_logic_interface.h"
#include "plugin_manager.h"
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>

class GameLogicManager {
private:
    std::vector<std::unique_ptr<IGameLogic >> activeLogics;
    std::unordered_map<std::string, GameLogicFactory> registeredFactories;
    std::unique_ptr<GameEngine::GameLogicPluginManager> pluginManager;
    mutable std::mutex mutex;
    bool initialized = false;

    // Register built-in game logic implementations
    void registerBuiltinLogics();

public:
    GameLogicManager() = default;
    ~GameLogicManager();

    // Initialize the manager
    bool initialize();

    // Shutdown and cleanup
    void shutdown();

    // Register a game logic factory
    void registerLogicFactory(const std::string& name, GameLogicFactory factory);

    // Create and activate a game logic instance
    bool createLogic(const std::string& name, entt::registry& registry);

    // Update all active game logics
    void update(entt::registry& registry, float deltaTime, const InputState& input);
    void fixedUpdate(entt::registry& registry, float fixedDeltaTime, const InputState& input);
    void lateUpdate(entt::registry& registry, float deltaTime, const InputState& input);

    // Event forwarding
    void onEntityCreated(entt::registry& registry, entt::entity entity);
    void onEntityDestroyed(entt::registry& registry, entt::entity entity);

    // Get active logics
    std::vector<std::string> getActiveLogics() const;

    // Remove a specific logic
    bool removeLogic(const std::string& name);

    // Clear all active logics
    void clearLogics();

    // Plugin management
    bool loadProjectPlugins(const std::string& projectPath);
    void unloadAllPlugins();

    // Check if initialized
    bool isInitialized() const { return initialized; }
};
