#pragma once

#include <entt/entt.hpp>

namespace GameEngine::Physics {

enum class BodyType {
    Static,
    Dynamic,
    Kinematic
};

struct RigidBody {
    BodyType type = BodyType::Dynamic;
    float mass = 1.0f;
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    bool fixedRotation = false;
    
    // Velocity
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float angularVelocity = 0.0f;
    
    // Forces
    float forceX = 0.0f;
    float forceY = 0.0f;
    float torque = 0.0f;
};

} // namespace GameEngine::Physics