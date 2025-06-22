#pragma once

#include <entt/entt.hpp>
#include <raylib.h>
#include <memory>
#include <functional>

class SpriteBatch;

// Render system that works without built-in components
// It provides hooks for plugins to register their rendering logic
class OptionalRenderSystem {
public:
    OptionalRenderSystem();
    ~OptionalRenderSystem();

    void initialize();
    void update(entt::registry& registry);
    void shutdown();

    // Plugin hooks
    using RenderCallback = std::function<void(entt::registry&, Camera2D&)>;
    void registerRenderCallback(const std::string& name, RenderCallback callback);
    void unregisterRenderCallback(const std::string& name);

    // Camera management (plugins can update this)
    Camera2D& getCamera() { return camera; }
    void setCamera(const Camera2D& cam) { camera = cam; }

private:
    Camera2D camera;
    std::unique_ptr<SpriteBatch> spriteBatch;
    std::unordered_map<std::string, RenderCallback> renderCallbacks;
};
