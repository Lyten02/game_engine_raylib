#include <iostream>
#include <filesystem>
#include "plugins/plugin_manager.h"
#include "plugins/plugin_api.h"
#include "packages/package_loader.h"

// Test helper macros
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAIL: " << message << std::endl; \
        return 1; \
    }

// Mock plugin for testing
class MockPlugin : public GameEngine::IPlugin {
public:
    bool onLoad(GameEngine::PluginAPI* api) override {
        loadCalled = true;
        this->api = api;
        return true;
    }
    
    void onUnload() override {
        unloadCalled = true;
    }
    
    GameEngine::PluginInfo getInfo() const override {
        return {
            "MockPlugin",
            "1.0.0",
            "Test plugin",
            "Test Author",
            PLUGIN_API_VERSION
        };
    }
    
    bool loadCalled = false;
    bool unloadCalled = false;
    GameEngine::PluginAPI* api = nullptr;
};

int main() {
    std::cout << "Running PluginManager tests..." << std::endl;
    
    // Test 1: Create PluginManager
    {
        std::cout << "\nTest 1: Create PluginManager..." << std::endl;
        
        GameEngine::PackageLoader loader;
        GameEngine::PluginManager manager(&loader);
        
        TEST_ASSERT(manager.getLoadedPlugins().empty(), "Should have no plugins loaded initially");
        
        std::cout << "PASS: PluginManager creation" << std::endl;
    }
    
    // Test 2: Load non-existent plugin
    {
        std::cout << "\nTest 2: Load non-existent plugin..." << std::endl;
        
        GameEngine::PackageLoader loader;
        GameEngine::PluginManager manager(&loader);
        
        bool result = manager.loadPlugin("non_existent_plugin.so");
        TEST_ASSERT(!result, "Should fail to load non-existent plugin");
        TEST_ASSERT(!manager.getLastError().empty(), "Should have error message");
        
        std::cout << "PASS: Non-existent plugin handling" << std::endl;
    }
    
    // Test 3: Check plugin loaded state
    {
        std::cout << "\nTest 3: Check plugin loaded state..." << std::endl;
        
        GameEngine::PackageLoader loader;
        GameEngine::PluginManager manager(&loader);
        
        TEST_ASSERT(!manager.isPluginLoaded("TestPlugin"), "Plugin should not be loaded");
        TEST_ASSERT(manager.getPluginInfo("TestPlugin") == nullptr, "Should return null for unloaded plugin");
        
        std::cout << "PASS: Plugin state checking" << std::endl;
    }
    
    // Test 4: Unload all plugins
    {
        std::cout << "\nTest 4: Unload all plugins..." << std::endl;
        
        GameEngine::PackageLoader loader;
        GameEngine::PluginManager manager(&loader);
        
        // Should not crash when no plugins loaded
        manager.unloadAllPlugins();
        TEST_ASSERT(manager.getLoadedPlugins().empty(), "Should still be empty");
        
        std::cout << "PASS: Unload all plugins" << std::endl;
    }
    
    std::cout << "\nAll PluginManager tests passed!" << std::endl;
    return 0;
}