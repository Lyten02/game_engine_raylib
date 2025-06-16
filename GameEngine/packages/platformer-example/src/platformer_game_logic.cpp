#include "platformer_game_logic.h"
#include <spdlog/spdlog.h>
#include <raylib.h>
#include <algorithm>

namespace PlatformerExample {

void PlatformerGameLogic::initialize(entt::registry& registry) {
    spdlog::info("PlatformerGameLogic initialized");
    
    // Find the player (GREEN colored entity)
    auto view = registry.view<TransformComponent, Sprite>();
    for (auto entity : view) {
        auto& sprite = view.get<Sprite>(entity);
        // Check if this is the player (green colored)
        if (sprite.tint.g > 200 && sprite.tint.r < 50 && sprite.tint.b < 50) {
            playerEntity = entity;
            spdlog::info("Player entity found (green sprite)");
            break;
        }
    }
    
    if (playerEntity == entt::null) {
        spdlog::warn("Player entity not found (looking for green sprite)");
    }
    
    // Create camera entity
    cameraEntity = registry.create();
    auto& camera = registry.emplace<CameraComponent>(cameraEntity);
    camera.target = {640.0f, 360.0f};
    camera.offset = {640.0f, 360.0f};
    camera.zoom = 1.0f;
    camera.active = true;
    
    spdlog::info("Camera entity created for platformer");
}

void PlatformerGameLogic::update(entt::registry& registry, float deltaTime) {
    if (playerEntity == entt::null || !registry.valid(playerEntity)) {
        return;
    }
    
    auto* transform = registry.try_get<TransformComponent>(playerEntity);
    if (!transform) return;
    
    // Input handling
    bool isMoving = false;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        playerPhysics.velocity.x = -moveSpeed;
        isMoving = true;
    } else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        playerPhysics.velocity.x = moveSpeed;
        isMoving = true;
    } else {
        playerPhysics.velocity.x *= 0.8f; // Friction
    }
    
    // Log movement once to avoid spam
    if (isMoving && !hasLoggedMovement) {
        spdlog::info("Player entity {} is moving", static_cast<uint32_t>(playerEntity));
        hasLoggedMovement = true;
    } else if (!isMoving) {
        hasLoggedMovement = false;
    }
    
    // Jumping
    if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) && playerPhysics.isGrounded) {
        playerPhysics.velocity.y = jumpForce;
        playerPhysics.isGrounded = false;
        spdlog::info("Player jumped");
    }
    
    // Apply gravity
    playerPhysics.velocity.y += gravity * deltaTime;
    
    // Limit fall speed
    if (playerPhysics.velocity.y > maxFallSpeed) {
        playerPhysics.velocity.y = maxFallSpeed;
    }
    
    // Update position
    transform->position.x += playerPhysics.velocity.x * deltaTime;
    transform->position.y += playerPhysics.velocity.y * deltaTime;
    
    // Ground and platform collision
    playerPhysics.isGrounded = false;
    auto allEntities = registry.view<TransformComponent, Sprite>();
    
    for (auto entity : allEntities) {
        if (entity == playerEntity) continue;
        
        auto& otherTransform = allEntities.get<TransformComponent>(entity);
        
        // Simple AABB collision
        float playerLeft = transform->position.x - transform->scale.x/2;
        float playerRight = transform->position.x + transform->scale.x/2;
        float playerTop = transform->position.y - transform->scale.y/2;
        float playerBottom = transform->position.y + transform->scale.y/2;
        
        float otherLeft = otherTransform.position.x - otherTransform.scale.x/2;
        float otherRight = otherTransform.position.x + otherTransform.scale.x/2;
        float otherTop = otherTransform.position.y - otherTransform.scale.y/2;
        float otherBottom = otherTransform.position.y + otherTransform.scale.y/2;
        
        // Check collision
        if (playerRight > otherLeft && playerLeft < otherRight &&
            playerBottom > otherTop && playerTop < otherBottom) {
            
            // Landing on top
            if (playerPhysics.velocity.y > 0 && playerBottom - otherTop < 20 && 
                transform->position.y < otherTransform.position.y) {
                transform->position.y = otherTop - transform->scale.y/2;
                playerPhysics.velocity.y = 0;
                playerPhysics.isGrounded = true;
            }
            // Hit from below
            else if (playerPhysics.velocity.y < 0 && otherBottom - playerTop < 20 &&
                     transform->position.y > otherTransform.position.y) {
                transform->position.y = otherBottom + transform->scale.y/2;
                playerPhysics.velocity.y = 0;
            }
            // Push from sides
            else {
                float overlapLeft = playerRight - otherLeft;
                float overlapRight = otherRight - playerLeft;
                
                if (overlapLeft < overlapRight) {
                    transform->position.x = otherLeft - transform->scale.x/2;
                } else {
                    transform->position.x = otherRight + transform->scale.x/2;
                }
                playerPhysics.velocity.x = 0;
            }
        }
    }
    
    // Keep player on screen bounds
    const float screenWidth = 1280.0f;
    const float screenHeight = 720.0f;
    transform->position.x = std::max(transform->scale.x/2, std::min(screenWidth - transform->scale.x/2, transform->position.x));
    
    // Reset if fallen too far
    if (transform->position.y > screenHeight + 100) {
        transform->position.x = 640;
        transform->position.y = 100;
        playerPhysics.velocity = {0, 0};
        spdlog::info("Player respawned");
    }
    
    // Update camera to follow player
    if (cameraEntity != entt::null && registry.valid(cameraEntity)) {
        auto* camera = registry.try_get<CameraComponent>(cameraEntity);
        if (camera) {
            // Smooth camera follow
            camera->target.x = transform->position.x;
            camera->target.y = transform->position.y;
            
            // Keep camera within bounds
            const float halfWidth = 640.0f;
            const float halfHeight = 360.0f;
            camera->target.x = std::max(halfWidth, std::min(screenWidth - halfWidth, camera->target.x));
            camera->target.y = std::max(halfHeight, std::min(screenHeight - halfHeight, camera->target.y));
        }
    }
}

void PlatformerGameLogic::shutdown() {
    spdlog::info("PlatformerGameLogic shutdown");
}

std::string PlatformerGameLogic::getName() const {
    return "PlatformerGameLogic";
}

} // namespace PlatformerExample