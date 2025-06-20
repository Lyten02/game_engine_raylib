#include <iostream>
#include <memory>
#include "plugins/plugin_interface.h"
#include "plugins/plugin_api.h"

// Example component that plugin provides
struct ExampleComponent {
    float speed = 100.0f;
    std::string tag = "example";
};

// Example system that plugin provides
class ExampleSystem : public GameEngine::ISystem {
public:
    void update(entt::registry& registry, float deltaTime) override {
        auto view = registry.view<ExampleComponent>();
        for (auto entity : view) {
            auto& comp = view.get<ExampleComponent>(entity);
            // Do something with the component
            comp.speed += deltaTime * 10.0f;
        }
    }
};

// The actual plugin implementation
class ExamplePlugin : public GameEngine::IPlugin {
public:
    bool onLoad(GameEngine::PluginAPI* api) override {
        if (!api) return false;
        
        api->log("ExamplePlugin loading...");
        
        // Register component
        api->registerComponent("ExampleComponent", [](entt::registry& reg, entt::entity entity) {
            reg.emplace<ExampleComponent>(entity);
        });
        
        // Register system
        api->registerSystem("ExampleSystem", []() -> std::unique_ptr<GameEngine::ISystem> {
            return std::make_unique<ExampleSystem>();
        });
        
        api->log("ExamplePlugin loaded successfully!");
        return true;
    }
    
    void onUnload() override {
        // Cleanup if needed
    }
    
    GameEngine::PluginInfo getInfo() const override {
        return {
            "ExamplePlugin",
            "1.0.0",
            "An example plugin demonstrating the plugin system",
            "GameEngine Team",
            PLUGIN_API_VERSION
        };
    }
};

// Implement the required export functions
IMPLEMENT_PLUGIN(ExamplePlugin)

// Test to verify the plugin works
int main() {
    std::cout << "Testing example plugin..." << std::endl;
    
    // Create plugin instance directly for testing
    auto plugin = createPlugin();
    
    // Check plugin info
    auto info = plugin->getInfo();
    std::cout << "Plugin: " << info.name << " v" << info.version << std::endl;
    std::cout << "Description: " << info.description << std::endl;
    
    // Clean up
    destroyPlugin(plugin);
    
    std::cout << "Plugin test passed!" << std::endl;
    return 0;
}