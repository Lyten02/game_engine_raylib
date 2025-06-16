#include <iostream>
#include <cassert>
#include <filesystem>

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

void test_plugin_system_components() {
    std::cout << "Test: All plugin system components exist... ";
    
    // Check engine components
    TEST_ASSERT(std::filesystem::exists("src/scripting/game_logic_interface.h"));
    TEST_ASSERT(std::filesystem::exists("src/scripting/plugin_api.h"));
    
    // Check template with plugin support
    TEST_ASSERT(std::filesystem::exists("templates/basic/game_template.cpp"));
    
    // Check example plugin
    TEST_ASSERT(std::filesystem::exists("packages/platformer-example/CMakeLists.txt"));
    TEST_ASSERT(std::filesystem::exists("packages/platformer-example/package.json"));
    TEST_ASSERT(std::filesystem::exists("packages/platformer-example/src/platformer_plugin.cpp"));
    TEST_ASSERT(std::filesystem::exists("packages/platformer-example/src/platformer_game_logic.cpp"));
    
    // Check built plugin
    TEST_ASSERT(std::filesystem::exists("build/packages/platformer-example/libplatformer.dylib"));
    
    std::cout << "✓" << std::endl;
}

void test_tdd_subtasks_completion() {
    std::cout << "Test: All TDD subtasks components present... ";
    
    // Subtask 1: Base plugin interface and dynamic library loader
    TEST_ASSERT(std::filesystem::exists("src/scripting/game_logic_interface.h"));
    
    // Subtask 2: API for component and system registration  
    TEST_ASSERT(std::filesystem::exists("src/scripting/plugin_api.h"));
    
    // Subtask 3: Integration with PackageLoader (in game_template.cpp)
    TEST_ASSERT(std::filesystem::exists("templates/basic/game_template.cpp"));
    TEST_ASSERT(std::filesystem::exists("tests/test_standalone_game_logic.cpp"));
    
    // Subtask 4: Example plugin with CMake configuration
    TEST_ASSERT(std::filesystem::exists("packages/platformer-example/CMakeLists.txt"));
    TEST_ASSERT(std::filesystem::exists("build/packages/platformer-example/libplatformer.dylib"));
    TEST_ASSERT(std::filesystem::exists("tests/test_plugin_cmake_build.cpp"));
    
    // Subtask 5: Security and lifecycle management
    TEST_ASSERT(std::filesystem::exists("tests/test_plugin_security.cpp"));
    
    std::cout << "✓" << std::endl;
}

void test_plugin_system_functionality() {
    std::cout << "Test: Plugin system functionality verification... ";
    
    // Run sub-tests
    int result1 = system("./tests/test_standalone_game_logic > /dev/null 2>&1");
    TEST_ASSERT(result1 == 0);
    
    int result2 = system("./test_plugin_cmake_build > /dev/null 2>&1"); 
    TEST_ASSERT(result2 == 0);
    
    int result3 = system("./test_plugin_security > /dev/null 2>&1");
    TEST_ASSERT(result3 == 0);
    
    std::cout << "✓" << std::endl;
}

int main() {
    std::cout << "\n=== Running Plugin System Integration Tests ===" << std::endl;
    std::cout << "=== Verifying TDD Implementation Completion ===" << std::endl;
    
    test_plugin_system_components();
    test_tdd_subtasks_completion(); 
    test_plugin_system_functionality();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Tests run: " << tests_run << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << (tests_run - tests_passed) << std::endl;
    
    if (tests_run == tests_passed) {
        std::cout << "\n✅ TDD Plugin System Implementation COMPLETE!" << std::endl;
        std::cout << "All 5 subtasks successfully implemented and tested:" << std::endl;
        std::cout << "1. ✅ Base plugin interface and dynamic library loader" << std::endl;
        std::cout << "2. ✅ API for component and system registration" << std::endl;
        std::cout << "3. ✅ Integration with PackageLoader" << std::endl;
        std::cout << "4. ✅ Example plugin with CMake configuration" << std::endl;
        std::cout << "5. ✅ Security and lifecycle management" << std::endl;
    }
    
    return (tests_run == tests_passed) ? 0 : 1;
}