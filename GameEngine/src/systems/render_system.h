#pragma once

#include <entt/fwd.hpp>

// Render System for Entity Component System
// Responsible for rendering all entities with Transform and Sprite components
class RenderSystem {
public:
    RenderSystem() = default;
    ~RenderSystem() = default;
    
    // Initialize the render system
    void initialize();
    
    // Update/render all entities with Transform and Sprite components
    void update(entt::registry& registry);
    
    // Shutdown and cleanup
    void shutdown();
};