#include "systems/render_system.h"
#include "components/transform.h"
#include "components/sprite.h"
#include <entt/entt.hpp>
#include <spdlog/spdlog.h>
#include <raylib.h>

void RenderSystem::initialize() {
    spdlog::info("RenderSystem::initialize - Render system initialized");
}

void RenderSystem::update(entt::registry& registry) {
    auto view = registry.view<TransformComponent, Sprite>();
    
    size_t spriteCount = 0;
    
    beginCamera();
    
    for (auto entity : view) {
        // Check if entity is still valid before accessing components
        if (!registry.valid(entity)) {
            continue;
        }
        
        // Use try_get to safely access components
        const auto* transform = registry.try_get<TransformComponent>(entity);
        const auto* sprite = registry.try_get<Sprite>(entity);
        
        // Check if both components exist
        if (!transform || !sprite) {
            continue;
        }
        
        if (sprite->texture == nullptr) {
            continue;
        }
        
        Vector2 position = { transform->position.x, transform->position.y };
        
        if (!testMode) {
            DrawTextureRec(*sprite->texture, sprite->sourceRect, position, sprite->tint);
        }
        
        spriteCount++;
    }
    
    endCamera();
    
    // Remove all logging from render loop to prevent FPS drops
    // Use console commands to get render statistics instead
}

void RenderSystem::shutdown() {
    spdlog::info("RenderSystem::shutdown - Render system shut down");
}

void RenderSystem::setCamera2D(Camera2D& cam) {
    camera = cam;
}

void RenderSystem::beginCamera() {
    BeginMode2D(camera);
}

void RenderSystem::endCamera() {
    EndMode2D();
}