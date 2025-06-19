#include <iostream>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "../src/packages/package_manager.h"

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

void createPackageWithDeps(const std::filesystem::path& dir, const std::string& name, 
                          const std::string& version, const nlohmann::json& deps) {
    std::filesystem::create_directories(dir / name);
    
    nlohmann::json packageJson;
    packageJson["name"] = name;
    packageJson["version"] = version;
    packageJson["description"] = "Test package " + name;
    packageJson["dependencies"] = deps;
    packageJson["components"] = nlohmann::json::array();
    packageJson["systems"] = nlohmann::json::array();
    
    std::ofstream file(dir / name / "package.json");
    file << packageJson.dump(2);
    file.close();
}

int main() {
    std::cout << "Running PackageManager dependency tests..." << std::endl;
    
    // Setup test directory
    std::filesystem::path testDir = std::filesystem::temp_directory_path() / "test_deps";
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);
    
    // Test 1: Load package with satisfied dependencies
    {
        std::cout << "\nTest 1: Load package with satisfied dependencies..." << std::endl;
        
        // Create dependency packages
        createPackageWithDeps(testDir, "math-utils", "1.0.0", nlohmann::json::object());
        createPackageWithDeps(testDir, "core", "2.0.0", nlohmann::json::object());
        
        // Create package that depends on them
        nlohmann::json deps;
        deps["math-utils"] = ">=1.0.0";
        deps["core"] = ">=2.0.0";
        createPackageWithDeps(testDir, "physics", "1.0.0", deps);
        
        GameEngine::PackageManager manager(testDir);
        
        // Load dependencies first
        TEST_ASSERT(manager.loadPackage("math-utils"), "Should load math-utils");
        TEST_ASSERT(manager.loadPackage("core"), "Should load core");
        
        // Now load package with dependencies
        TEST_ASSERT(manager.loadPackage("physics"), "Should load physics with satisfied deps");
        
        // Check dependency resolution
        auto resolved = manager.checkDependencies("physics");
        TEST_ASSERT(resolved.satisfied, "All dependencies should be satisfied");
        TEST_ASSERT(resolved.missing.empty(), "No dependencies should be missing");
        
        std::cout << "PASS: Satisfied dependencies" << std::endl;
    }
    
    // Test 2: Load package with missing dependencies
    {
        std::cout << "\nTest 2: Load package with missing dependencies..." << std::endl;
        
        std::filesystem::path testDir2 = testDir / "test2";
        std::filesystem::create_directories(testDir2);
        
        // Create package with unsatisfied dependencies
        nlohmann::json deps;
        deps["non-existent"] = ">=1.0.0";
        deps["missing-lib"] = "^2.0.0";
        createPackageWithDeps(testDir2, "app", "1.0.0", deps);
        
        GameEngine::PackageManager manager(testDir2);
        
        // Try to load package
        bool loaded = manager.loadPackage("app");
        // Should still load but warn about dependencies
        TEST_ASSERT(loaded, "Package should load even with missing deps (warn only)");
        
        // Check dependency resolution
        auto resolved = manager.checkDependencies("app");
        TEST_ASSERT(!resolved.satisfied, "Dependencies should not be satisfied");
        TEST_ASSERT_EQ(size_t(2), resolved.missing.size(), "Should have 2 missing dependencies");
        
        std::cout << "PASS: Missing dependencies detection" << std::endl;
    }
    
    // Test 3: Version compatibility in dependencies
    {
        std::cout << "\nTest 3: Version compatibility in dependencies..." << std::endl;
        
        std::filesystem::path testDir3 = testDir / "test3";
        std::filesystem::create_directories(testDir3);
        
        // Create packages with specific versions
        createPackageWithDeps(testDir3, "lib-a", "1.5.0", nlohmann::json::object());
        createPackageWithDeps(testDir3, "lib-b", "2.0.0", nlohmann::json::object());
        
        // Create package requiring specific versions
        nlohmann::json deps;
        deps["lib-a"] = ">=1.0.0";  // Should be satisfied by 1.5.0
        deps["lib-b"] = "^2.0.0";   // Should be satisfied by 2.0.0
        createPackageWithDeps(testDir3, "app", "1.0.0", deps);
        
        GameEngine::PackageManager manager(testDir3);
        
        // Load all packages
        manager.loadPackage("lib-a");
        manager.loadPackage("lib-b");
        manager.loadPackage("app");
        
        auto resolved = manager.checkDependencies("app");
        TEST_ASSERT(resolved.satisfied, "Version requirements should be satisfied");
        
        // Test incompatible version
        nlohmann::json deps2;
        deps2["lib-a"] = ">=2.0.0";  // Not satisfied by 1.5.0
        createPackageWithDeps(testDir3, "app2", "1.0.0", deps2);
        
        manager.loadPackage("app2");
        auto resolved2 = manager.checkDependencies("app2");
        TEST_ASSERT(!resolved2.satisfied, "Version requirement should not be satisfied");
        TEST_ASSERT_EQ(size_t(1), resolved2.incompatible.size(), "Should have 1 incompatible version");
        
        std::cout << "PASS: Version compatibility" << std::endl;
    }
    
    // Test 4: Circular dependencies detection
    {
        std::cout << "\nTest 4: Circular dependency detection..." << std::endl;
        
        std::filesystem::path testDir4 = testDir / "test4";
        std::filesystem::create_directories(testDir4);
        
        // Create circular dependencies: A -> B -> C -> A
        nlohmann::json depsA, depsB, depsC;
        depsA["pkg-b"] = "1.0.0";
        depsB["pkg-c"] = "1.0.0";
        depsC["pkg-a"] = "1.0.0";
        
        createPackageWithDeps(testDir4, "pkg-a", "1.0.0", depsA);
        createPackageWithDeps(testDir4, "pkg-b", "1.0.0", depsB);
        createPackageWithDeps(testDir4, "pkg-c", "1.0.0", depsC);
        
        GameEngine::PackageManager manager(testDir4);
        
        // Try to load with circular deps
        bool hasCircular = manager.hasCircularDependency("pkg-a");
        TEST_ASSERT(hasCircular, "Should detect circular dependency");
        
        std::cout << "PASS: Circular dependency detection" << std::endl;
    }
    
    // Test 5: Dependency loading order
    {
        std::cout << "\nTest 5: Dependency loading order..." << std::endl;
        
        std::filesystem::path testDir5 = testDir / "test5";
        std::filesystem::create_directories(testDir5);
        
        // Create dependency chain: app -> lib1 -> lib2
        createPackageWithDeps(testDir5, "lib2", "1.0.0", nlohmann::json::object());
        
        nlohmann::json lib1Deps;
        lib1Deps["lib2"] = "1.0.0";
        createPackageWithDeps(testDir5, "lib1", "1.0.0", lib1Deps);
        
        nlohmann::json appDeps;
        appDeps["lib1"] = "1.0.0";
        createPackageWithDeps(testDir5, "app", "1.0.0", appDeps);
        
        GameEngine::PackageManager manager(testDir5);
        
        // Get loading order
        auto order = manager.getDependencyOrder("app");
        TEST_ASSERT_EQ(size_t(3), order.size(), "Should have 3 packages in order");
        TEST_ASSERT_EQ("lib2", order[0], "lib2 should be loaded first");
        TEST_ASSERT_EQ("lib1", order[1], "lib1 should be loaded second");
        TEST_ASSERT_EQ("app", order[2], "app should be loaded last");
        
        std::cout << "PASS: Dependency loading order" << std::endl;
    }
    
    // Test 6: Load all dependencies automatically
    {
        std::cout << "\nTest 6: Automatic dependency loading..." << std::endl;
        
        std::filesystem::path testDir6 = testDir / "test6";
        std::filesystem::create_directories(testDir6);
        
        // Create dependency tree
        createPackageWithDeps(testDir6, "base", "1.0.0", nlohmann::json::object());
        
        nlohmann::json utilDeps;
        utilDeps["base"] = "1.0.0";
        createPackageWithDeps(testDir6, "utils", "1.0.0", utilDeps);
        
        nlohmann::json appDeps;
        appDeps["utils"] = "1.0.0";
        createPackageWithDeps(testDir6, "myapp", "1.0.0", appDeps);
        
        GameEngine::PackageManager manager(testDir6);
        manager.scanPackages();
        
        // Load package with dependencies
        bool loaded = manager.loadPackageWithDependencies("myapp");
        TEST_ASSERT(loaded, "Should load package with all dependencies");
        
        // Check all are loaded
        TEST_ASSERT(manager.getPackage("base") != nullptr, "base should be loaded");
        TEST_ASSERT(manager.getPackage("utils") != nullptr, "utils should be loaded");
        TEST_ASSERT(manager.getPackage("myapp") != nullptr, "myapp should be loaded");
        
        std::cout << "PASS: Automatic dependency loading" << std::endl;
    }
    
    // Cleanup
    std::filesystem::remove_all(testDir);
    
    std::cout << "\nAll dependency tests passed!" << std::endl;
    return 0;
}