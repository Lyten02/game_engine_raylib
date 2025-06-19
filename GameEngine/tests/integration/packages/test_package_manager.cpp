#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include "../src/packages/package_manager.h"
#include "../src/packages/package.h"

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
    std::cout << "Running PackageManager tests..." << std::endl;
    
    // Setup test directory
    std::filesystem::path testDir = std::filesystem::temp_directory_path() / "test_packages";
    std::filesystem::remove_all(testDir);  // Clean up from previous runs
    std::filesystem::create_directories(testDir);
    
    // Test 1: Package class creation
    {
        std::cout << "\nTest 1: Package creation..." << std::endl;
        Package package("test-package", "1.0.0");
        
        TEST_ASSERT_EQ("test-package", package.getName(), "Package name mismatch");
        TEST_ASSERT_EQ("1.0.0", package.getVersion(), "Package version mismatch");
        TEST_ASSERT(package.getDependencies().empty(), "Package should have no dependencies");
        std::cout << "PASS: Package creation" << std::endl;
    }
    
    // Test 2: Package with dependencies
    {
        std::cout << "\nTest 2: Package dependencies..." << std::endl;
        Package package("physics-2d", "1.0.0");
        package.addDependency("math-utils", ">=1.0.0");
        
        auto deps = package.getDependencies();
        TEST_ASSERT_EQ(size_t(1), deps.size(), "Package should have 1 dependency");
        TEST_ASSERT_EQ("math-utils", deps[0].name, "Dependency name mismatch");
        TEST_ASSERT_EQ(">=1.0.0", deps[0].version, "Dependency version mismatch");
        std::cout << "PASS: Package dependencies" << std::endl;
    }
    
    // Test 3: PackageManager creation
    {
        std::cout << "\nTest 3: PackageManager creation..." << std::endl;
        PackageManager manager(testDir);
        
        TEST_ASSERT_EQ(testDir, manager.getPackagesDirectory(), "Package directory mismatch");
        TEST_ASSERT(manager.getLoadedPackages().empty(), "Manager should have no loaded packages");
        std::cout << "PASS: PackageManager creation" << std::endl;
    }
    
    // Test 4: Scanning packages directory
    {
        std::cout << "\nTest 4: Scanning packages directory..." << std::endl;
        
        // Create mock package directories with package.json files
        std::filesystem::create_directories(testDir / "physics-2d");
        std::filesystem::create_directories(testDir / "animation");
        
        // Create package.json for physics-2d
        std::ofstream physicsJson(testDir / "physics-2d" / "package.json");
        physicsJson << R"({"name": "physics-2d", "version": "1.0.0"})";
        physicsJson.close();
        
        // Create package.json for animation
        std::ofstream animationJson(testDir / "animation" / "package.json");
        animationJson << R"({"name": "animation", "version": "1.0.0"})";
        animationJson.close();
        
        PackageManager manager(testDir);
        manager.scanPackages();
        
        auto availablePackages = manager.getAvailablePackages();
        TEST_ASSERT_EQ(size_t(2), availablePackages.size(), "Should find 2 packages");
        
        auto hasPhysics = std::find(availablePackages.begin(), availablePackages.end(), "physics-2d") != availablePackages.end();
        auto hasAnimation = std::find(availablePackages.begin(), availablePackages.end(), "animation") != availablePackages.end();
        
        TEST_ASSERT(hasPhysics, "Should find physics-2d package");
        TEST_ASSERT(hasAnimation, "Should find animation package");
        std::cout << "PASS: Package scanning" << std::endl;
    }
    
    // Test 5: Loading a package
    {
        std::cout << "\nTest 5: Loading a package..." << std::endl;
        
        // Create a mock package
        auto packageDir = testDir / "test-package";
        std::filesystem::create_directories(packageDir);
        
        // Create package.json
        std::ofstream packageJson(packageDir / "package.json");
        packageJson << R"({
            "name": "test-package",
            "version": "1.0.0",
            "description": "Test package"
        })";
        packageJson.close();
        
        PackageManager manager(testDir);
        bool result = manager.loadPackage("test-package");
        
        TEST_ASSERT(result, "Package loading should succeed");
        TEST_ASSERT_EQ(size_t(1), manager.getLoadedPackages().size(), "Should have 1 loaded package");
        TEST_ASSERT(manager.getPackage("test-package") != nullptr, "Should be able to get loaded package");
        std::cout << "PASS: Package loading" << std::endl;
    }
    
    // Test 6: Loading non-existent package
    {
        std::cout << "\nTest 6: Loading non-existent package..." << std::endl;
        
        PackageManager manager(testDir);
        bool result = manager.loadPackage("non-existent");
        
        TEST_ASSERT(!result, "Loading non-existent package should fail");
        TEST_ASSERT(manager.getLoadedPackages().empty(), "Should have no loaded packages");
        std::cout << "PASS: Non-existent package handling" << std::endl;
    }
    
    // Test 7: Getting package info
    {
        std::cout << "\nTest 7: Getting package info..." << std::endl;
        
        // Create a mock package
        auto packageDir = testDir / "info-test";
        std::filesystem::create_directories(packageDir);
        
        std::ofstream packageJson(packageDir / "package.json");
        packageJson << R"({
            "name": "info-test",
            "version": "2.0.1",
            "description": "Package for testing info"
        })";
        packageJson.close();
        
        PackageManager manager(testDir);
        manager.loadPackage("info-test");
        
        auto info = manager.getPackageInfo("info-test");
        TEST_ASSERT(info.has_value(), "Should get package info");
        TEST_ASSERT_EQ("info-test", info->name, "Package info name mismatch");
        TEST_ASSERT_EQ("2.0.1", info->version, "Package info version mismatch");
        TEST_ASSERT_EQ("Package for testing info", info->description, "Package info description mismatch");
        std::cout << "PASS: Package info retrieval" << std::endl;
    }
    
    // Cleanup
    std::filesystem::remove_all(testDir);
    
    std::cout << "\nAll PackageManager tests passed!" << std::endl;
    return 0;
}