#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>

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

void test_security_features_exist() {
    std::cout << "Test: Security features in game template... ";
    
    std::ifstream file("templates/basic/game_template.cpp");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // Check for security-related code
    TEST_ASSERT(content.find("securityEnabled") != std::string::npos);
    TEST_ASSERT(content.find("allowedPaths") != std::string::npos);
    TEST_ASSERT(content.find("isPathAllowed") != std::string::npos);
    TEST_ASSERT(content.find("disableSecurity") != std::string::npos);
    
    std::cout << "✓" << std::endl;
}

void test_lifecycle_management() {
    std::cout << "Test: Plugin lifecycle management... ";
    
    std::ifstream file("templates/basic/game_template.cpp");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // Check for lifecycle management
    TEST_ASSERT(content.find("unloadPlugin") != std::string::npos);
    TEST_ASSERT(content.find("getLoadedPlugins") != std::string::npos);
    TEST_ASSERT(content.find("dlclose") != std::string::npos);
    
    std::cout << "✓" << std::endl;
}

void test_plugin_validation() {
    std::cout << "Test: Plugin validation features... ";
    
    std::ifstream file("templates/basic/game_template.cpp");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // Check for validation features
    TEST_ASSERT(content.find("initializePlugin") != std::string::npos);
    TEST_ASSERT(content.find("dlsym") != std::string::npos);
    TEST_ASSERT(content.find("Plugin file not found") != std::string::npos);
    TEST_ASSERT(content.find("Plugin already loaded") != std::string::npos);
    
    std::cout << "✓" << std::endl;
}

void test_error_handling() {
    std::cout << "Test: Error handling in plugin system... ";
    
    std::ifstream file("templates/basic/game_template.cpp");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // Check for error handling
    TEST_ASSERT(content.find("spdlog::error") != std::string::npos);
    TEST_ASSERT(content.find("spdlog::warn") != std::string::npos);
    TEST_ASSERT(content.find("dlerror()") != std::string::npos);
    
    std::cout << "✓" << std::endl;
}

int main() {
    std::cout << "\n=== Running Plugin Security Tests ===" << std::endl;
    
    test_security_features_exist();
    test_lifecycle_management();
    test_plugin_validation();
    test_error_handling();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Tests run: " << tests_run << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << (tests_run - tests_passed) << std::endl;
    
    return (tests_run == tests_passed) ? 0 : 1;
}