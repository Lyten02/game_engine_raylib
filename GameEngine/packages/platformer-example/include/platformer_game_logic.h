#pragma once

#include <game_logic_interface.h>
#include <components/transform.h>
#include <components/sprite.h>
#include <components/camera.h>
#include <entt/entt.hpp>
#include <raylib.h>

namespace PlatformerExample {

// Platform types
enum class PlatformType {
    SOLID,    // Can't pass through from any direction
    ONE_WAY   // Can pass through from below
};

// Simple physics component for platformer
struct SimplePhysics {
    Vector2 velocity = {0, 0};
    bool isGrounded = false;
};

// Platform component
struct PlatformComponent {
    PlatformType type = PlatformType::SOLID;
};

class PlatformerGameLogic : public IGameLogic {
private:
    entt::entity playerEntity = entt::null;
    entt::entity cameraEntity = entt::null;
    SimplePhysics playerPhysics;
    
    // Physics constants
    const float gravity = 1000.0f;
    const float jumpForce = -500.0f;
    const float moveSpeed = 300.0f;
    const float maxFallSpeed = 1000.0f;
    
    bool hasLoggedMovement = false;
    int framesSinceLastLog = 0;
    const int LOG_INTERVAL = 300; // Log every 5 seconds at 60fps

public:
    void initialize(entt::registry& registry) override;
    void update(entt::registry& registry, float deltaTime, const InputState& input) override;
    void shutdown() override;
    std::string getName() const override;
};

} // namespace PlatformerExample