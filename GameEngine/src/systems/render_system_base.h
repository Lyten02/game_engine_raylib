#pragma once

#include <entt/entt.hpp>
#include <memory>

// Base render system interface that doesn't depend on any components
class IRenderSystem {
public:
    virtual ~IRenderSystem() = default;
    virtual void initialize() = 0;
    virtual void update(entt::registry& registry) = 0;
    virtual void shutdown() = 0;
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
};

// Factory for creating render systems
class RenderSystemFactory {
public:
    static std::unique_ptr<IRenderSystem> createDefault();
    static std::unique_ptr<IRenderSystem> createEmpty();
};
