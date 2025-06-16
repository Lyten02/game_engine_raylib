#pragma once

#include <entt/fwd.hpp>
#include <raylib.h>

class RenderSystem {
private:
    Camera2D camera;

protected:
    bool testMode = false;

public:
    RenderSystem() = default;
    virtual ~RenderSystem() = default;
    
    void initialize();
    void update(entt::registry& registry);
    void shutdown();
    
    void setCamera2D(Camera2D& camera);
    virtual void beginCamera();
    virtual void endCamera();
    
    void setTestMode(bool enabled) { testMode = enabled; }
};