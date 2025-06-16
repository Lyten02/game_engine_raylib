#include "systems/render_system.h"
#include "render/sprite_batch.h"
#include <entt/entt.hpp>
#include <spdlog/spdlog.h>
#include <raylib.h>

RenderSystem::RenderSystem() : spriteBatch(std::make_unique<SpriteBatch>()) {
    // Initialize camera with default values
    camera.target = {640.0f, 360.0f};
    camera.offset = {640.0f, 360.0f};  // Center of screen
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    spdlog::info("[RenderSystem] Camera initialized with default values");
}

RenderSystem::~RenderSystem() = default;

void RenderSystem::initialize() {
    spdlog::info("RenderSystem::initialize - Render system initialized (no built-in components)");
}

void RenderSystem::update(entt::registry& registry) {
    // RenderSystem is now just a placeholder
    // All rendering is handled by plugins that register their own components
    // This maintains compatibility but does nothing
    
    BeginMode2D(camera);
    // Plugins will handle actual rendering via their own systems
    EndMode2D();
}

void RenderSystem::shutdown() {
    spdlog::info("RenderSystem::shutdown - Render system shut down");
}

bool RenderSystem::isEnabled() const {
    return enabled;
}

void RenderSystem::setEnabled(bool value) {
    enabled = value;
    spdlog::info("RenderSystem::setEnabled - Render system {}", value ? "enabled" : "disabled");
}

void RenderSystem::setCameraTarget(float x, float y) {
    camera.target = {x, y};
}

void RenderSystem::setCameraOffset(float x, float y) {
    camera.offset = {x, y};
}

void RenderSystem::setCameraRotation(float rotation) {
    camera.rotation = rotation;
}

void RenderSystem::setCameraZoom(float zoom) {
    camera.zoom = zoom;
}

Camera2D RenderSystem::getCamera() const {
    return camera;
}

void RenderSystem::setCamera2D(Camera2D& newCamera) {
    camera = newCamera;
}

void RenderSystem::beginCamera() {
    BeginMode2D(camera);
}

void RenderSystem::endCamera() {
    EndMode2D();
}