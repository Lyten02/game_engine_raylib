#pragma once

#include <entt/entt.hpp>

namespace GameEngine {

// Base interface for all game systems
class ISystem {
public:
    virtual ~ISystem() = default;
    
    // Called once when system is initialized
    virtual void initialize() {}
    
    // Called every frame
    virtual void update(entt::registry& registry, float deltaTime) = 0;
    
    // Called when system is shut down
    virtual void shutdown() {}
};

} // namespace GameEngine