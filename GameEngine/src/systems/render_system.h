#pragma once

#include <entt/fwd.hpp>
#include <raylib.h>

class RenderSystem {
private:
    Camera2D camera;

public:
    RenderSystem() = default;
    ~RenderSystem() = default;
    
    void initialize();
    void update(entt::registry& registry);
    void shutdown();
    
    void setCamera2D(Camera2D& camera);
    void beginCamera();
    void endCamera();
};