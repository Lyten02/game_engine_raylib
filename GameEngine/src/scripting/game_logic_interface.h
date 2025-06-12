#pragma once

#include <string>
#include <memory>
#include <entt/entity/registry.hpp>

// Base interface for game logic implementations
class IGameLogic {
public:
    virtual ~IGameLogic() = default;
    
    // Lifecycle methods
    virtual void initialize(entt::registry& registry) = 0;
    virtual void update(entt::registry& registry, float deltaTime) = 0;
    virtual void fixedUpdate(entt::registry& registry, float fixedDeltaTime) {}
    virtual void lateUpdate(entt::registry& registry, float deltaTime) {}
    virtual void shutdown() = 0;
    
    // Event handling
    virtual void onEntityCreated(entt::registry& registry, entt::entity entity) {}
    virtual void onEntityDestroyed(entt::registry& registry, entt::entity entity) {}
    
    // Configuration
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const { return "1.0.0"; }
};

// Factory function type for creating game logic instances
using GameLogicFactory = std::unique_ptr<IGameLogic>(*)();

// Macro for easy game logic registration
#define REGISTER_GAME_LOGIC(className) \
    std::unique_ptr<IGameLogic> createGameLogic() { \
        return std::make_unique<className>(); \
    }