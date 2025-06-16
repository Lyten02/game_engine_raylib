#include "optional_render_system.h"
#include "render/sprite_batch.h"
#include <spdlog/spdlog.h>

OptionalRenderSystem::OptionalRenderSystem() 
    : spriteBatch(std::make_unique<SpriteBatch>()) {
    // Initialize default 2D camera
    camera.target = {640.0f, 360.0f};
    camera.offset = {640.0f, 360.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
}

OptionalRenderSystem::~OptionalRenderSystem() = default;

void OptionalRenderSystem::initialize() {
    spdlog::info("OptionalRenderSystem: Initialized (no built-in components)");
}

void OptionalRenderSystem::update(entt::registry& registry) {
    BeginMode2D(camera);
    
    // Call all registered render callbacks
    for (const auto& [name, callback] : renderCallbacks) {
        callback(registry, camera);
    }
    
    EndMode2D();
}

void OptionalRenderSystem::shutdown() {
    renderCallbacks.clear();
    spdlog::info("OptionalRenderSystem: Shutdown complete");
}

void OptionalRenderSystem::registerRenderCallback(const std::string& name, RenderCallback callback) {
    renderCallbacks[name] = callback;
    spdlog::info("OptionalRenderSystem: Registered render callback '{}'", name);
}

void OptionalRenderSystem::unregisterRenderCallback(const std::string& name) {
    renderCallbacks.erase(name);
    spdlog::info("OptionalRenderSystem: Unregistered render callback '{}'", name);
}