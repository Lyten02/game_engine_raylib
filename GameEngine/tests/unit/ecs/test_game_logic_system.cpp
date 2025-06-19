#include <iostream>
#include <cassert>
#include "../src/scripting/game_logic_interface.h"
#include "../src/components/transform.h"
#include <entt/entity/registry.hpp>

// Simple stub for spdlog
namespace spdlog {
    inline void info(const std::string& msg) { std::cout << "[INFO] " << msg << std::endl; }
    inline void error(const std::string& msg) { std::cerr << "[ERROR] " << msg << std::endl; }
    inline void warn(const std::string& msg) { std::cout << "[WARN] " << msg << std::endl; }
    template<typename... Args>
    inline void info(const std::string& fmt, Args... args) { std::cout << "[INFO] " << fmt << std::endl; }
    template<typename... Args>
    inline void error(const std::string& fmt, Args... args) { std::cerr << "[ERROR] " << fmt << std::endl; }
}

#include "../src/scripting/game_logic_manager.h"

// Test game logic implementation
class TestGameLogic : public IGameLogic {
private:
    int updateCount = 0;
    bool initialized = false;

public:
    void initialize(entt::registry& registry) override {
        initialized = true;
        std::cout << "TestGameLogic initialized" << std::endl;
    }
    
    void update(entt::registry& registry, float deltaTime) override {
        updateCount++;
    }
    
    void shutdown() override {
        std::cout << "TestGameLogic shutdown after " << updateCount << " updates" << std::endl;
    }
    
    std::string getName() const override {
        return "TestGameLogic";
    }
    
    int getUpdateCount() const { return updateCount; }
    bool isInitialized() const { return initialized; }
};

// Factory function
std::unique_ptr<IGameLogic> createTestGameLogic() {
    return std::make_unique<TestGameLogic>();
}

void test_game_logic_manager() {
    std::cout << "Testing GameLogicManager..." << std::endl;
    
    GameLogicManager manager;
    entt::registry registry;
    
    // Test initialization
    assert(manager.initialize());
    assert(manager.isInitialized());
    
    // Test registering factory
    manager.registerLogicFactory("TestLogic", createTestGameLogic);
    
    // Test creating logic
    assert(manager.createLogic("TestLogic", registry));
    
    // Test active logics
    auto activeLogics = manager.getActiveLogics();
    assert(activeLogics.size() == 1);
    assert(activeLogics[0] == "TestGameLogic");
    
    // Test update
    manager.update(registry, 0.016f);
    manager.update(registry, 0.016f);
    manager.update(registry, 0.016f);
    
    // Test shutdown
    manager.shutdown();
    
    std::cout << "✓ GameLogicManager test passed" << std::endl;
}

void test_game_logic_interface() {
    std::cout << "Testing IGameLogic interface..." << std::endl;
    
    entt::registry registry;
    TestGameLogic logic;
    
    // Test initialization
    logic.initialize(registry);
    assert(logic.isInitialized());
    
    // Test update
    logic.update(registry, 0.016f);
    assert(logic.getUpdateCount() == 1);
    
    // Test name
    assert(logic.getName() == "TestGameLogic");
    
    // Test version
    assert(logic.getVersion() == "1.0.0");
    
    // Test shutdown
    logic.shutdown();
    
    std::cout << "✓ IGameLogic interface test passed" << std::endl;
}

int main() {
    std::cout << "=== C++ Game Logic System Tests ===" << std::endl;
    
    try {
        test_game_logic_interface();
        test_game_logic_manager();
        
        std::cout << "\nAll tests passed! ✓" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}