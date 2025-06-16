#include "render_system_base.h"
#include <spdlog/spdlog.h>

// Empty render system that does nothing - for when no rendering components are loaded
class EmptyRenderSystem : public IRenderSystem {
private:
    bool enabled = true;
    
public:
    void initialize() override {
        spdlog::info("EmptyRenderSystem: Initialized (no rendering will occur)");
    }
    
    void update(entt::registry& registry) override {
        // Do nothing - no rendering components available
    }
    
    void shutdown() override {
        spdlog::info("EmptyRenderSystem: Shutting down");
    }
    
    bool isEnabled() const override {
        return enabled;
    }
    
    void setEnabled(bool value) override {
        enabled = value;
    }
};

// Default render system factory implementation
std::unique_ptr<IRenderSystem> RenderSystemFactory::createEmpty() {
    return std::make_unique<EmptyRenderSystem>();
}

// For now, default is empty until core-components plugin loads
std::unique_ptr<IRenderSystem> RenderSystemFactory::createDefault() {
    return createEmpty();
}