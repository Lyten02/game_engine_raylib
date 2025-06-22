#include <iostream>
#include <cassert>
#include <memory>
#include "engine.h"
#include "systems/render_system.h"
#include "resources/resource_manager.h"
#include "console/console.h"
#include "scripting/game_logic_manager.h"

// Simple test framework
#define REQUIRE(condition) do { if (!(condition)) { std::cerr << "FAILED: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; exit(1); } } while(0)
#define CHECK(condition) do { if (!(condition)) { std::cerr << "CHECK FAILED: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; } } while(0)

void test_engine_basic_initialization() {
    std::cout << "  Testing basic initialization..." << std::endl;
    Engine engine;
    engine.setHeadlessMode(true);
    
    REQUIRE(engine.initialize() == true);
    // In headless mode, render system is not initialized
    REQUIRE(engine.getRenderSystem() == nullptr);
    REQUIRE(engine.getResourceManager() != nullptr);
    REQUIRE(engine.getConsole() != nullptr);
    REQUIRE(engine.getCommandProcessor() != nullptr);
    REQUIRE(engine.getScriptManager() != nullptr);  // Stub for compatibility
    REQUIRE(engine.getGameLogicManager() != nullptr);
    REQUIRE(engine.getProjectManager() != nullptr);
    REQUIRE(engine.getCurrentScene() != nullptr);
    
    engine.shutdown();
    std::cout << "  ✅ Basic initialization test passed" << std::endl;
}

void test_engine_headless_mode() {
    std::cout << "  Testing headless mode..." << std::endl;
    Engine engine;
    
    engine.setHeadlessMode(true);
    REQUIRE(engine.isHeadlessMode() == true);
    REQUIRE(engine.initialize() == true);
    
    // Run should return immediately in headless mode
    engine.run();
    
    engine.shutdown();
    std::cout << "  ✅ Headless mode test passed" << std::endl;
}

void test_engine_multiple_init_shutdown() {
    std::cout << "  Testing multiple init/shutdown cycles..." << std::endl;
    Engine engine;
    engine.setHeadlessMode(true);
    
    for (int i = 0; i < 3; i++) {
        REQUIRE(engine.initialize() == true);
        REQUIRE(engine.getCurrentScene() != nullptr);
        engine.shutdown();
    }
    
    std::cout << "  ✅ Multiple init/shutdown test passed" << std::endl;
}

void test_engine_destructor_safety() {
    std::cout << "  Testing destructor safety..." << std::endl;
    
    // Test initialized engine destruction
    {
        Engine engine;
        engine.setHeadlessMode(true);
        REQUIRE(engine.initialize() == true);
        // Destructor called here
    }
    
    // Test uninitialized engine destruction
    {
        Engine engine;
        // Destructor called here without initialization
    }
    
    std::cout << "  ✅ Destructor safety test passed" << std::endl;
}

void test_engine_scene_management() {
    std::cout << "  Testing scene management..." << std::endl;
    Engine engine;
    engine.setHeadlessMode(true);
    
    REQUIRE(engine.initialize() == true);
    REQUIRE(engine.getCurrentScene() != nullptr);
    
    // Destroy scene
    engine.destroyScene();
    REQUIRE(engine.getCurrentScene() == nullptr);
    
    // Create new scene
    engine.createScene();
    REQUIRE(engine.getCurrentScene() != nullptr);
    
    engine.shutdown();
    std::cout << "  ✅ Scene management test passed" << std::endl;
}

void test_engine_memory_stress() {
    std::cout << "  Testing memory stress..." << std::endl;
    
    // Sequential creation and destruction
    for (int i = 0; i < 10; i++) {
        auto engine = std::make_unique<Engine>();
        engine->setHeadlessMode(true);
        REQUIRE(engine->initialize() == true);
        engine->shutdown();
    }
    
    std::cout << "  ✅ Memory stress test passed" << std::endl;
}

int main() {
    std::cout << "🧪 Running Engine initialization tests..." << std::endl;
    std::cout << "========================================" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    // Run tests
    struct Test {
        std::string name;
        void (*func)();
    };
    
    Test tests[] = {
        {"engine_basic_initialization", test_engine_basic_initialization},
        {"engine_headless_mode", test_engine_headless_mode},
        {"engine_multiple_init_shutdown", test_engine_multiple_init_shutdown},
        {"engine_destructor_safety", test_engine_destructor_safety},
        {"engine_scene_management", test_engine_scene_management},
        {"engine_memory_stress", test_engine_memory_stress}
    };
    
    for (const auto& test : tests) {
        std::cout << "\n📋 " << test.name << std::endl;
        try {
            test.func();
            passed++;
        } catch (const std::exception& e) {
            std::cerr << "  ❌ Exception: " << e.what() << std::endl;
            failed++;
        } catch (...) {
            std::cerr << "  ❌ Unknown exception" << std::endl;
            failed++;
        }
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    
    if (failed == 0) {
        std::cout << "✅ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "❌ Some tests failed!" << std::endl;
        return 1;
    }
}