#include <iostream>
#include <cassert>
#include <filesystem>
#include <dlfcn.h>

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

void test_plugin_library_exists() {
    std::cout << "Test: Plugin library exists... ";
    
    std::filesystem::path pluginPath = "build/packages/platformer-example/libplatformer.dylib";
    TEST_ASSERT(std::filesystem::exists(pluginPath));
    
    std::cout << "✓" << std::endl;
}

void test_plugin_library_loadable() {
    std::cout << "Test: Plugin library is loadable... ";
    
    std::filesystem::path pluginPath = "build/packages/platformer-example/libplatformer.dylib";
    
    if (std::filesystem::exists(pluginPath)) {
        void* handle = dlopen(pluginPath.c_str(), RTLD_LAZY);
        TEST_ASSERT(handle != nullptr);
        
        if (handle) {
            // Check for required exports
            void* initFunc = dlsym(handle, "initializePlugin");
            TEST_ASSERT(initFunc != nullptr);
            
            void* nameFunc = dlsym(handle, "getPluginName");
            TEST_ASSERT(nameFunc != nullptr);
            
            dlclose(handle);
        }
    } else {
        TEST_ASSERT(false); // Library doesn't exist
    }
    
    std::cout << "✓" << std::endl;
}

void test_plugin_cmake_config() {
    std::cout << "Test: Plugin CMakeConfig exists... ";
    
    std::filesystem::path cmakeFile = "packages/platformer-example/CMakeLists.txt";
    TEST_ASSERT(std::filesystem::exists(cmakeFile));
    
    std::cout << "✓" << std::endl;
}

void test_package_json_config() {
    std::cout << "Test: Package JSON config exists... ";
    
    std::filesystem::path packageJson = "packages/platformer-example/package.json";
    TEST_ASSERT(std::filesystem::exists(packageJson));
    
    std::cout << "✓" << std::endl;
}

int main() {
    std::cout << "\n=== Running Plugin CMake Build Tests ===" << std::endl;
    
    test_plugin_cmake_config();
    test_package_json_config();
    test_plugin_library_exists();
    test_plugin_library_loadable();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Tests run: " << tests_run << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << (tests_run - tests_passed) << std::endl;
    
    return (tests_run == tests_passed) ? 0 : 1;
}