#pragma once

#include "game_logic_interface.h"
#include <vector>
#include <unordered_map>
#include <mutex>

class GameLogicManager {
private:
    std::vector<std::unique_ptr<IGameLogic>> activeLogics;
    std::unordered_map<std::string, GameLogicFactory> registeredFactories;
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
    void update(entt::registry& registry, float deltaTime);
    void fixedUpdate(entt::registry& registry, float fixedDeltaTime);
    void lateUpdate(entt::registry& registry, float deltaTime);
    
    // Event forwarding
    void onEntityCreated(entt::registry& registry, entt::entity entity);
    void onEntityDestroyed(entt::registry& registry, entt::entity entity);
    
    // Get active logics
    std::vector<std::string> getActiveLogics() const;
    
    // Remove a specific logic
    bool removeLogic(const std::string& name);
    
    // Clear all active logics
    void clearLogics();
    
    // Check if initialized
    bool isInitialized() const { return initialized; }
};