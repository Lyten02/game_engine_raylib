#pragma once

#include <entt/entt.hpp>

namespace GameEngine::Physics {

class PhysicsSystem {
public:
    PhysicsSystem() = default;
    ~PhysicsSystem() = default;
    
    void initialize();
    void update(entt::registry& registry, float deltaTime);
    void shutdown();
    
    // Physics settings
    void setGravity(float x, float y);
    void setTimeStep(float timeStep);
    
private:
    float gravityX = 0.0f;
    float gravityY = -9.81f;
    float timeStep = 1.0f / 60.0f;
    float accumulator = 0.0f;
};

} // namespace GameEngine::Physics