#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <nlohmann/json.hpp>

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition) do { \
    tests_run++; \
    if (!(condition)) { \
        std::cerr << "❌ Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "   Condition: " << #condition << std::endl; \
    } else { \
        tests_passed++; \
    } \
} while(0)

// Mock game config structure
struct GameConfig {
    std::string projectName;
    std::string gameLogic;
    std::vector<std::string> dependencies;
    std::string startScene;
    
    bool loadFromJson(const nlohmann::json& json) {
        projectName = json.value("name", "");
        gameLogic = json.value("game_logic", "");
        startScene = json.value("start_scene", "main");
        
        if (json.contains("dependencies") && json["dependencies"].is_array()) {
            for (const auto& dep : json["dependencies"]) {
                dependencies.push_back(dep);
            }
        }
        
        return !projectName.empty();
    }
};

void test_load_game_config() {
    std::cout << "Test: Load game config from JSON... ";
    
    nlohmann::json testJson = {
        {"name", "TestPlatformer"},
        {"game_logic", "PlatformerGameLogic"},
        {"dependencies", {"platformer-example"}},
        {"start_scene", "level1"}
    };
    
    GameConfig config;
    TEST_ASSERT(config.loadFromJson(testJson));
    TEST_ASSERT(config.projectName == "TestPlatformer");
    TEST_ASSERT(config.gameLogic == "PlatformerGameLogic");
    TEST_ASSERT(config.dependencies.size() == 1);
    TEST_ASSERT(config.dependencies[0] == "platformer-example");
    TEST_ASSERT(config.startScene == "level1");
    
    std::cout << "✓" << std::endl;
}

void test_empty_game_logic() {
    std::cout << "Test: Handle empty game logic... ";
    
    nlohmann::json testJson = {
        {"name", "SimpleGame"}
    };
    
    GameConfig config;
    TEST_ASSERT(config.loadFromJson(testJson));
    TEST_ASSERT(config.gameLogic.empty());
    TEST_ASSERT(config.dependencies.empty());
    
    std::cout << "✓" << std::endl;
}

void test_plugin_path_resolution() {
    std::cout << "Test: Plugin path resolution... ";
    
    // Test resolving plugin library path
    std::string packageName = "platformer-example";
    std::string libraryName = "libplatformer.dylib";
    
    // Expected path pattern
    std::filesystem::path expectedPath = std::filesystem::path("packages") / packageName / libraryName;
    
    TEST_ASSERT(expectedPath.string() == "packages/platformer-example/libplatformer.dylib");
    
    std::cout << "✓" << std::endl;
}

void test_game_logic_factory_registration() {
    std::cout << "Test: Game logic factory registration simulation... ";
    
    // Simulate a factory registry
    std::unordered_map<std::string, bool> mockRegistry;
    
    // Simulate registering a game logic
    std::string logicName = "PlatformerGameLogic";
    mockRegistry[logicName] = true;
    
    // Test that it was registered
    TEST_ASSERT(mockRegistry.find(logicName) != mockRegistry.end());
    TEST_ASSERT(mockRegistry[logicName] == true);
    
    // Test non-existent logic
    TEST_ASSERT(mockRegistry.find("NonExistentLogic") == mockRegistry.end());
    
    std::cout << "✓" << std::endl;
}

int main() {
    std::cout << "\n=== Running Standalone Game Logic Tests ===\n" << std::endl;
    
    test_load_game_config();
    test_empty_game_logic();
    test_plugin_path_resolution();
    test_game_logic_factory_registration();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Tests run: " << tests_run << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << (tests_run - tests_passed) << std::endl;
    
    return (tests_run == tests_passed) ? 0 : 1;
}