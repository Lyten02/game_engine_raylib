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
        const auto& transform = view.get<TransformComponent>(entity);
        const auto& sprite = view.get<Sprite>(entity);
        
        if (sprite.texture == nullptr) {
            continue;
        }
        
        Vector2 position = { transform.position.x, transform.position.y };
        
        DrawTextureRec(*sprite.texture, sprite.sourceRect, position, sprite.tint);
        
        spriteCount++;
    }
    
    endCamera();
    
    static int frameCounter = 0;
    if (frameCounter++ % 60 == 0) {
        spdlog::info("RenderSystem: FPS: {}, Sprites rendered: {}", GetFPS(), spriteCount);
    }
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