#include "platformer_game_logic.h"
#include <spdlog/spdlog.h>
#include <raylib.h>
#include <algorithm>

namespace PlatformerExample {

void PlatformerGameLogic::initialize(entt::registry& registry) {
    spdlog::info("=== PlatformerGameLogic::initialize() CALLED ===");
    
    // Count all entities
    auto allView = registry.view<entt::entity>();
    int totalEntities = 0;
    for (auto entity : allView) {
        totalEntities++;
    }
    spdlog::info("Total entities in registry: {}", totalEntities);
    
    // Find the player (GREEN colored entity)
    auto view = registry.view<TransformComponent, Sprite>();
    int entityCount = 0;
    for (auto entity : view) {
        auto& sprite = view.get<Sprite>(entity);
        entityCount++;
        spdlog::info("Entity {}: color R={} G={} B={}", static_cast<uint32_t>(entity), 
                    sprite.tint.r, sprite.tint.g, sprite.tint.b);
        
        // Check if this is the player (green colored)
        if (sprite.tint.g > 200 && sprite.tint.r < 50 && sprite.tint.b < 50) {
            playerEntity = entity;
            spdlog::info("Player entity found (green sprite): {}", static_cast<uint32_t>(entity));
            break;
        }
    }
    
    spdlog::info("Found {} entities with Transform+Sprite components", entityCount);
    
    if (playerEntity == entt::null) {
        spdlog::warn("Player entity not found (looking for green sprite)");
    }
    
    // Create camera entity
    cameraEntity = registry.create();
    auto& camera = registry.emplace<CameraComponent>(cameraEntity);
    camera.target = {400.0f, 300.0f};
    camera.offset = {640.0f, 360.0f}; // Center of screen
    camera.zoom = 1.0f;
    camera.active = true;
    
    // Debug: log current camera state
    if (cameraEntity != entt::null && registry.valid(cameraEntity)) {
        auto* camera = registry.try_get<CameraComponent>(cameraEntity);
        if (camera) {
            spdlog::info("Camera after creation - active: {}, target: ({}, {}), offset: ({}, {})",
                        camera->active, camera->target.x, camera->target.y, 
                        camera->offset.x, camera->offset.y);
        }
    }
    
    // Tag ALL entities except player as platforms
    auto allEntities = registry.view<TransformComponent, Sprite>();
    int platformCount = 0;
    for (auto entity : allEntities) {
        if (entity == playerEntity) continue;
        
        auto& sprite = allEntities.get<Sprite>(entity);
        auto& transform = allEntities.get<TransformComponent>(entity);
        
        // Make some platforms one-way based on position
        if (transform.position.y < 350) { // Upper platforms
            registry.emplace<PlatformComponent>(entity, PlatformType::ONE_WAY);
            // Make one-way platforms slightly transparent
            sprite.tint.a = 180;
            spdlog::info("Created ONE_WAY platform {} at position ({}, {})", 
                        platformCount++, transform.position.x, transform.position.y);
        } else { // Lower platforms are solid
            registry.emplace<PlatformComponent>(entity, PlatformType::SOLID);
            spdlog::info("Created SOLID platform {} at position ({}, {})", 
                        platformCount++, transform.position.x, transform.position.y);
        }
    }
    spdlog::info("Total platforms created: {}", platformCount);
    
    spdlog::info("Camera entity created for platformer: {}", static_cast<uint32_t>(cameraEntity));
    spdlog::info("=== PlatformerGameLogic::initialize() COMPLETE ===");
}

void PlatformerGameLogic::update(entt::registry& registry, float deltaTime, const InputState& input) {
    framesSinceLastLog++;
    
    if (playerEntity == entt::null || !registry.valid(playerEntity)) {
        spdlog::warn("Player entity invalid");
        return;
    }
    
    auto* transform = registry.try_get<TransformComponent>(playerEntity);
    if (!transform) {
        spdlog::warn("Player entity missing TransformComponent");
        return;
    }
    
    // Input handling
    bool isMoving = false;
    
    // Debug: log only significant events
    if (framesSinceLastLog >= LOG_INTERVAL) {
        spdlog::info("Player pos: ({:.1f}, {:.1f}), velocity: ({:.1f}, {:.1f}), grounded: {}",
                    transform->position.x, transform->position.y,
                    playerPhysics.velocity.x, playerPhysics.velocity.y,
                    playerPhysics.isGrounded);
        framesSinceLastLog = 0;
    }
    
    if (input.isKeyDown(KEY_LEFT) || input.isKeyDown(KEY_A)) {
        playerPhysics.velocity.x = -moveSpeed;
        isMoving = true;
        // Removed spam log
    } else if (input.isKeyDown(KEY_RIGHT) || input.isKeyDown(KEY_D)) {
        playerPhysics.velocity.x = moveSpeed;
        isMoving = true;
        // Removed spam log
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
    if ((input.isKeyPressed(KEY_SPACE) || input.isKeyPressed(KEY_UP) || input.isKeyPressed(KEY_W)) && playerPhysics.isGrounded) {
        playerPhysics.velocity.y = jumpForce;
        playerPhysics.isGrounded = false;
        spdlog::info("Player jumped from position ({:.1f}, {:.1f})", transform->position.x, transform->position.y);
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
    auto platformView = registry.view<TransformComponent, PlatformComponent, Sprite>();
    
    // Debug log platforms
    static int collisionLogCount = 0;
    if (++collisionLogCount % 120 == 0) {
        int platformCount = 0;
        for (auto e : platformView) {
            platformCount++;
        }
        spdlog::info("Checking collision with {} platforms", platformCount);
    }
    
    for (auto entity : platformView) {
        auto& otherTransform = platformView.get<TransformComponent>(entity);
        auto& platform = platformView.get<PlatformComponent>(entity);
        auto& otherSprite = platformView.get<Sprite>(entity);
        
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
            
            // Handle collision based on platform type
            if (platform.type == PlatformType::ONE_WAY) {
                // One-way platform - only collide from above when falling
                if (playerPhysics.velocity.y > 0 && playerBottom - otherTop < 20 && 
                    transform->position.y < otherTransform.position.y) {
                    transform->position.y = otherTop - transform->scale.y/2;
                    playerPhysics.velocity.y = 0;
                    playerPhysics.isGrounded = true;
                }
            } else { // SOLID platform
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
    }
    
    // Keep player on screen bounds
    const float screenWidth = 1280.0f;
    const float screenHeight = 720.0f;
    transform->position.x = std::max(transform->scale.x/2, std::min(screenWidth - transform->scale.x/2, transform->position.x));
    
    // Debug platform collision check
    if (framesSinceLastLog >= LOG_INTERVAL - 1) {
        auto debugPlatforms = registry.view<PlatformComponent>();
        int platformsWithComponent = 0;
        for (auto e : debugPlatforms) {
            platformsWithComponent++;
        }
        spdlog::info("Platforms with PlatformComponent: {}", platformsWithComponent);
    }
    
    // Reset if fallen too far
    if (transform->position.y > screenHeight + 100) {
        transform->position.x = 640;
        transform->position.y = 100;
        playerPhysics.velocity = {0, 0};
        spdlog::info("Player respawned");
    }
    
    // Update camera to follow player with smooth interpolation
    if (cameraEntity != entt::null && registry.valid(cameraEntity)) {
        auto* camera = registry.try_get<CameraComponent>(cameraEntity);
        if (camera) {
            // Direct camera follow - no smoothing for now to debug
            camera->target.x = transform->position.x;
            camera->target.y = transform->position.y;
            
            // Debug camera values
            static int cameraLogCount = 0;
            if (++cameraLogCount % 60 == 0) {
                spdlog::info("Camera active: {}, target: ({:.1f}, {:.1f}), Player: ({:.1f}, {:.1f}), offset: ({:.1f}, {:.1f})",
                            camera->active, camera->target.x, camera->target.y,
                            transform->position.x, transform->position.y,
                            camera->offset.x, camera->offset.y);
            }
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