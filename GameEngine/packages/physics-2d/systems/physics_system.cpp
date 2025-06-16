#include "physics_system.h"
#include "../components/rigid_body.h"
#include "../components/box_collider.h"
#include "../../../src/components/transform_component.h"
#include <spdlog/spdlog.h>

namespace GameEngine::Physics {

void PhysicsSystem::initialize() {
    spdlog::info("[PhysicsSystem] Initializing with gravity: ({}, {})", gravityX, gravityY);
}

void PhysicsSystem::update(entt::registry& registry, float deltaTime) {
    // Simple physics integration
    auto view = registry.view<RigidBody, TransformComponent>();
    
    for (auto entity : view) {
        auto& rb = view.get<RigidBody>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        
        if (rb.type == BodyType::Static) {
            continue;
        }
        
        // Apply gravity
        if (rb.type == BodyType::Dynamic) {
            rb.forceY += rb.mass * gravityY;
        }
        
        // Update velocity from forces
        rb.velocityX += (rb.forceX / rb.mass) * deltaTime;
        rb.velocityY += (rb.forceY / rb.mass) * deltaTime;
        rb.angularVelocity += (rb.torque / rb.mass) * deltaTime;
        
        // Apply damping
        rb.velocityX *= (1.0f - rb.linearDamping * deltaTime);
        rb.velocityY *= (1.0f - rb.linearDamping * deltaTime);
        rb.angularVelocity *= (1.0f - rb.angularDamping * deltaTime);
        
        // Update position from velocity
        transform.position.x += rb.velocityX * deltaTime;
        transform.position.y += rb.velocityY * deltaTime;
        
        if (!rb.fixedRotation) {
            transform.rotation.z += rb.angularVelocity * deltaTime;
        }
        
        // Clear forces
        rb.forceX = 0.0f;
        rb.forceY = 0.0f;
        rb.torque = 0.0f;
    }
}

void PhysicsSystem::shutdown() {
    spdlog::info("[PhysicsSystem] Shutting down");
}

void PhysicsSystem::setGravity(float x, float y) {
    gravityX = x;
    gravityY = y;
    spdlog::debug("[PhysicsSystem] Gravity set to: ({}, {})", x, y);
}

void PhysicsSystem::setTimeStep(float ts) {
    timeStep = ts;
    spdlog::debug("[PhysicsSystem] Time step set to: {}", ts);
}

} // namespace GameEngine::Physics