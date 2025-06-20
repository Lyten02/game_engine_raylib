#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include "packages/package_manager.h"
#include "packages/package.h"

using GameEngine::Package;
using GameEngine::PackageManager;

// Test helper macros
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAIL: " << message << std::endl; \
        return 1; \
    }

#define TEST_ASSERT_EQ(expected, actual, message) \
    if ((expected) != (actual)) { \
        std::cerr << "FAIL: " << message << ". Expected: " << expected << ", Actual: " << actual << std::endl; \
        return 1; \
    }

int main() {
    std::cout << "Running PackageManager metadata tests..." << std::endl;
    
    // Setup test directory
    std::filesystem::path testDir = std::filesystem::temp_directory_path() / "test_packages_metadata";
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);
    
    // Test 1: Load package with full metadata
    {
        std::cout << "\nTest 1: Loading package with full metadata..." << std::endl;
        
        // Create a mock package with all metadata
        auto packageDir = testDir / "full-package";
        std::filesystem::create_directories(packageDir);
        
        // Create comprehensive package.json
        std::ofstream packageJson(packageDir / "package.json");
        packageJson << R"({
            "name": "full-package",
            "version": "2.1.0",
            "description": "A complete test package",
            "author": "Test Author",
            "license": "MIT",
            "engineVersion": ">=0.1.0",
            "dependencies": {
                "physics-2d": ">=1.0.0",
                "math-utils": "^1.2.0"
            },
            "components": [
                {
                    "name": "TestComponent",
                    "file": "components/test_component.h"
                },
                {
                    "name": "AnotherComponent",
                    "file": "components/another_component.h"
                }
            ],
            "systems": [
                {
                    "name": "TestSystem",
                    "file": "systems/test_system.h",
                    "priority": 100
                },
                {
                    "name": "LowPrioritySystem",
                    "file": "systems/low_priority_system.h",
                    "priority": 10
                }
            ]
        })";
        packageJson.close();
        
        PackageManager manager(testDir);
        bool result = manager.loadPackage("full-package");
        
        TEST_ASSERT(result, "Package loading should succeed");
        
        auto package = manager.getPackage("full-package");
        TEST_ASSERT(package != nullptr, "Should get loaded package");
        
        // Verify basic metadata
        TEST_ASSERT_EQ("full-package", package->getName(), "Package name mismatch");
        TEST_ASSERT_EQ("2.1.0", package->getVersion(), "Package version mismatch");
        TEST_ASSERT_EQ("A complete test package", package->getDescription(), "Package description mismatch");
        TEST_ASSERT_EQ("Test Author", package->getAuthor(), "Package author mismatch");
        TEST_ASSERT_EQ("MIT", package->getLicense(), "Package license mismatch");
        TEST_ASSERT_EQ(">=0.1.0", package->getEngineVersion(), "Package engine version mismatch");
        
        // Verify dependencies
        auto deps = package->getDependencies();
        TEST_ASSERT_EQ(size_t(2), deps.size(), "Should have 2 dependencies");
        
        // Verify components
        auto components = package->getComponents();
        TEST_ASSERT_EQ(size_t(2), components.size(), "Should have 2 components");
        TEST_ASSERT_EQ("TestComponent", components[0].name, "First component name mismatch");
        TEST_ASSERT_EQ("components/test_component.h", components[0].file, "First component file mismatch");
        
        // Verify systems
        auto systems = package->getSystems();
        TEST_ASSERT_EQ(size_t(2), systems.size(), "Should have 2 systems");
        TEST_ASSERT_EQ("TestSystem", systems[0].name, "First system name mismatch");
        TEST_ASSERT_EQ(100, systems[0].priority, "First system priority mismatch");
        
        std::cout << "PASS: Full metadata loading" << std::endl;
    }
    
    // Test 2: Load real engine packages
    {
        std::cout << "\nTest 2: Loading real engine packages..." << std::endl;
        
        // Point to real packages directory
        std::filesystem::path packagesDir = "../packages";
        
        if (std::filesystem::exists(packagesDir)) {
            PackageManager manager(packagesDir);
            manager.scanPackages();
            
            auto availablePackages = manager.getAvailablePackages();
            std::cout << "Found " << availablePackages.size() << " real packages" << std::endl;
            
            // Try to load physics-2d package if it exists
            if (std::find(availablePackages.begin(), availablePackages.end(), "physics-2d") != availablePackages.end()) {
                bool loaded = manager.loadPackage("physics-2d");
                TEST_ASSERT(loaded, "Should load physics-2d package");
                
                auto physics = manager.getPackage("physics-2d");
                TEST_ASSERT(physics != nullptr, "Should get physics package");
                
                // Verify physics package has components and systems
                auto components = physics->getComponents();
                auto systems = physics->getSystems();
                
                std::cout << "Physics package has " << components.size() << " components and " 
                         << systems.size() << " systems" << std::endl;
                
                TEST_ASSERT(components.size() > 0, "Physics package should have components");
                TEST_ASSERT(systems.size() > 0, "Physics package should have systems");
            }
            
            std::cout << "PASS: Real package loading" << std::endl;
        } else {
            std::cout << "SKIP: Real packages directory not found" << std::endl;
        }
    }
    
    // Test 3: Version validation
    {
        std::cout << "\nTest 3: Engine version validation..." << std::endl;
        
        // Create package with specific engine version requirement
        auto packageDir = testDir / "version-test";
        std::filesystem::create_directories(packageDir);
        
        std::ofstream packageJson(packageDir / "package.json");
        packageJson << R"({
            "name": "version-test",
            "version": "1.0.0",
            "description": "Version test package",
            "engineVersion": ">=0.1.0"
        })";
        packageJson.close();
        
        PackageManager manager(testDir);
        bool result = manager.loadPackage("version-test");
        
        TEST_ASSERT(result, "Package with engine version should load");
        
        auto package = manager.getPackage("version-test");
        TEST_ASSERT_EQ(">=0.1.0", package->getEngineVersion(), "Engine version requirement mismatch");
        
        std::cout << "PASS: Engine version handling" << std::endl;
    }
    
    // Cleanup
    std::filesystem::remove_all(testDir);
    
    std::cout << "\nAll PackageManager metadata tests passed!" << std::endl;
    return 0;
}