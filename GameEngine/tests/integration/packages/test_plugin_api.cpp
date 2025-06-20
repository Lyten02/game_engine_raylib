#include <iostream>
#include <memory>
#include "plugins/plugin_api.h"
#include "plugins/plugin_interface.h"
#include "packages/package_loader.h"

// Test helper macros
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAIL: " << message << std::endl; \
        return 1; \
    }

// Test component
struct TestComponent {
    int value = 42;
    std::string name = "test";
};

// Test system
class TestSystem : public GameEngine::ISystem {
public:
    void update(entt::registry& registry, float deltaTime) override {
        updateCalled = true;
        lastDeltaTime = deltaTime;
    }
    
    bool updateCalled = false;
    float lastDeltaTime = 0.0f;
};


int main() {
    std::cout << "Running PluginAPI tests..." << std::endl;
    
    // Test 1: Create PluginAPI
    {
        std::cout << "\nTest 1: Create PluginAPI..." << std::endl;
        
        GameEngine::PackageLoader loader;
        GameEngine::PluginAPI api(&loader);
        
        TEST_ASSERT(api.getEngineAPIVersion() == PLUGIN_API_VERSION, "Should return correct API version");
        
        std::cout << "PASS: PluginAPI creation" << std::endl;
    }
    
    // Test 2: Register component
    {
        std::cout << "\nTest 2: Register component..." << std::endl;
        
        GameEngine::PackageLoader loader;
        GameEngine::PluginAPI api(&loader);
        
        // Register a component
        api.registerComponent("TestComponent", [](entt::registry& reg, entt::entity entity) {
            reg.emplace<TestComponent>(entity);
        });
        
        TEST_ASSERT(loader.hasComponent("TestComponent"), "Should have registered TestComponent");
        TEST_ASSERT(loader.getRegisteredComponents().size() == 1, "Should have 1 component");
        
        // Test the factory
        auto factory = loader.getComponentFactory("TestComponent");
        TEST_ASSERT(factory != nullptr, "Should get component factory");
        
        entt::registry registry;
        auto entity = registry.create();
        factory(registry, entity);
        
        TEST_ASSERT(registry.all_of<TestComponent>(entity), "Entity should have TestComponent");
        
        std::cout << "PASS: Component registration" << std::endl;
    }
    
    // Test 3: Register system
    {
        std::cout << "\nTest 3: Register system..." << std::endl;
        
        GameEngine::PackageLoader loader;
        GameEngine::PluginAPI api(&loader);
        
        // Register a system
        api.registerSystem("TestSystem", []() -> std::unique_ptr<GameEngine::ISystem> {
            return std::make_unique<TestSystem>();
        });
        
        TEST_ASSERT(loader.hasSystem("TestSystem"), "Should have registered TestSystem");
        TEST_ASSERT(loader.getRegisteredSystems().size() == 1, "Should have 1 system");
        
        // Test the factory
        auto factory = loader.getSystemFactory("TestSystem");
        TEST_ASSERT(factory != nullptr, "Should get system factory");
        
        auto system = factory();
        TEST_ASSERT(system != nullptr, "Factory should create system");
        
        // Test system functionality
        entt::registry registry;
        system->update(registry, 0.016f);
        
        auto testSystem = dynamic_cast<TestSystem*>(system.get());
        TEST_ASSERT(testSystem != nullptr, "Should be TestSystem");
        TEST_ASSERT(testSystem->updateCalled, "Update should have been called");
        TEST_ASSERT(testSystem->lastDeltaTime == 0.016f, "Delta time should match");
        
        std::cout << "PASS: System registration" << std::endl;
    }
    
    // Test 4: Logging functions
    {
        std::cout << "\nTest 4: Logging functions..." << std::endl;
        
        GameEngine::PackageLoader loader;
        GameEngine::PluginAPI api(&loader);
        
        // These should not crash
        api.log("Test log message");
        api.logWarning("Test warning");
        api.logError("Test error");
        
        std::cout << "PASS: Logging functions" << std::endl;
    }
    
    // Test 5: Multiple registrations
    {
        std::cout << "\nTest 5: Multiple registrations..." << std::endl;
        
        GameEngine::PackageLoader loader;
        GameEngine::PluginAPI api(&loader);
        
        // Register multiple components
        api.registerComponent("Component1", [](entt::registry& reg, entt::entity entity) {});
        api.registerComponent("Component2", [](entt::registry& reg, entt::entity entity) {});
        api.registerComponent("Component3", [](entt::registry& reg, entt::entity entity) {});
        
        // Register multiple systems
        api.registerSystem("System1", []() { return std::make_unique<TestSystem>(); });
        api.registerSystem("System2", []() { return std::make_unique<TestSystem>(); });
        
        TEST_ASSERT(loader.getRegisteredComponents().size() == 3, "Should have 3 components");
        TEST_ASSERT(loader.getRegisteredSystems().size() == 2, "Should have 2 systems");
        
        std::cout << "PASS: Multiple registrations" << std::endl;
    }
    
    std::cout << "\nAll PluginAPI tests passed!" << std::endl;
    return 0;
}