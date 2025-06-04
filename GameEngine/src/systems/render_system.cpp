#include "systems/render_system.h"
#include "components/transform.h"
#include "components/sprite.h"
#include <entt/entt.hpp>
#include <spdlog/spdlog.h>

void RenderSystem::initialize() {
    spdlog::info("RenderSystem::initialize - Render system initialized");
}

void RenderSystem::update(entt::registry& registry) {
    // Get a view of all entities that have both Transform and Sprite components
    auto view = registry.view<TransformComponent, Sprite>();
    
    // Count entities for logging
    size_t entityCount = 0;
    
    // Iterate through all entities with Transform and Sprite components
    for (auto entity : view) {
        // Get the components for this entity
        const auto& transform = view.get<TransformComponent>(entity);
        const auto& sprite = view.get<Sprite>(entity);
        
        // TODO: Implement sprite rendering
        // This will involve:
        // 1. Using the transform position to determine where to draw
        // 2. Using the sprite texture and source rectangle
        // 3. Applying the tint color
        // 4. Handling rotation and scale from transform
        
        entityCount++;
    }
    
    // Log the number of entities processed (only log occasionally to avoid spam)
    static int frameCounter = 0;
    if (frameCounter++ % 60 == 0) { // Log every 60 frames (once per second at 60 FPS)
        spdlog::debug("RenderSystem::update - Processed {} entities", entityCount);
    }
}

void RenderSystem::shutdown() {
    spdlog::info("RenderSystem::shutdown - Render system shut down");
}