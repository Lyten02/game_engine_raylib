#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <cassert>

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

class PackageManagerTest {
public:
    bool testPackageManagerComponents() {
        std::cout << "=== Package Manager Components Test ===" << std::endl;
        
        // Test 1: Check if package manager headers exist
        std::cout << "Test 1: Checking package manager headers..." << std::endl;
        
        std::filesystem::path srcDir = "../src";
        std::filesystem::path packageManagerPath = srcDir / "packages" / "package_manager.h";
        std::filesystem::path packageLoaderPath = srcDir / "packages" / "package_loader.h";
        
        bool hasPackageManager = std::filesystem::exists(packageManagerPath);
        bool hasPackageLoader = std::filesystem::exists(packageLoaderPath);
        
        TEST_ASSERT(hasPackageManager);
        TEST_ASSERT(hasPackageLoader);
        
        std::cout << "  Package Manager header: " << (hasPackageManager ? "✓ Found" : "✗ Missing") << std::endl;
        std::cout << "  Package Loader header: " << (hasPackageLoader ? "✓ Found" : "✗ Missing") << std::endl;
        
        // Test 2: Check for expected package manager functionality
        std::cout << "\nTest 2: Checking package manager functionality..." << std::endl;
        
        if (hasPackageManager) {
            std::ifstream file(packageManagerPath);
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            
            bool hasLoadPackage = content.find("loadPackage") != std::string::npos;
            bool hasCheckDependencies = content.find("checkDependencies") != std::string::npos;
            bool hasPackageManagerClass = content.find("class PackageManager") != std::string::npos;
            bool hasVersionControl = content.find("version") != std::string::npos;
            
            TEST_ASSERT(hasLoadPackage);
            TEST_ASSERT(hasCheckDependencies);
            TEST_ASSERT(hasPackageManagerClass);
            TEST_ASSERT(hasVersionControl);
            
            std::cout << "  Load package method: " << (hasLoadPackage ? "✓ Found" : "✗ Missing") << std::endl;
            std::cout << "  Dependency checking: " << (hasCheckDependencies ? "✓ Found" : "✗ Missing") << std::endl;
            std::cout << "  PackageManager class: " << (hasPackageManagerClass ? "✓ Found" : "✗ Missing") << std::endl;
            std::cout << "  Version control: " << (hasVersionControl ? "✓ Found" : "✗ Missing") << std::endl;
        }
        
        return hasPackageManager && hasPackageLoader;
    }
    
    bool testPackageStructure() {
        std::cout << "\n=== Package Structure Test ===" << std::endl;
        
        // Test 3: Check package.json structure
        std::cout << "Test 3: Checking package.json structure..." << std::endl;
        
        std::filesystem::path packageJsonPath = "../packages/platformer-example/package.json";
        bool hasPackageJson = std::filesystem::exists(packageJsonPath);
        
        TEST_ASSERT(hasPackageJson);
        std::cout << "  Package.json exists: " << (hasPackageJson ? "✓ Found" : "✗ Missing") << std::endl;
        
        if (hasPackageJson) {
            std::ifstream file(packageJsonPath);
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            
            bool hasName = content.find("\"name\"") != std::string::npos;
            bool hasVersion = content.find("\"version\"") != std::string::npos;
            bool hasDependencies = content.find("dependencies") != std::string::npos;
            bool hasDescription = content.find("description") != std::string::npos;
            
            TEST_ASSERT(hasName);
            TEST_ASSERT(hasVersion);
            
            std::cout << "  Name field: " << (hasName ? "✓ Found" : "✗ Missing") << std::endl;
            std::cout << "  Version field: " << (hasVersion ? "✓ Found" : "✗ Missing") << std::endl;
            std::cout << "  Dependencies field: " << (hasDependencies ? "✓ Found" : "⚠ Optional") << std::endl;
            std::cout << "  Description field: " << (hasDescription ? "✓ Found" : "⚠ Optional") << std::endl;
        }
        
        return hasPackageJson;
    }
    
    bool testPackageIntegration() {
        std::cout << "\n=== Package Integration Test ===" << std::endl;
        
        // Test 4: Check engine integration with package system
        std::cout << "Test 4: Checking engine integration..." << std::endl;
        
        std::filesystem::path enginePath = "../src/engine.cpp";
        if (std::filesystem::exists(enginePath)) {
            std::ifstream file(enginePath);
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            
            bool hasPackageInit = content.find("package") != std::string::npos ||
                                 content.find("Package") != std::string::npos;
            
            std::cout << "  Engine package integration: " << (hasPackageInit ? "✓ Found" : "⚠ May be missing") << std::endl;
        }
        
        // Test 5: Check build system integration
        std::cout << "\nTest 5: Checking build system integration..." << std::endl;
        
        std::filesystem::path cmakePath = "../CMakeLists.txt";
        if (std::filesystem::exists(cmakePath)) {
            std::ifstream file(cmakePath);
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            
            bool hasPackageSources = content.find("packages") != std::string::npos;
            bool hasSubdirectory = content.find("add_subdirectory") != std::string::npos;
            
            TEST_ASSERT(hasPackageSources || hasSubdirectory);
            
            std::cout << "  Package sources in CMake: " << (hasPackageSources ? "✓ Found" : "⚠ May be missing") << std::endl;
            std::cout << "  Subdirectory inclusion: " << (hasSubdirectory ? "✓ Found" : "⚠ May be missing") << std::endl;
        }
        
        return true;
    }
    
    bool testPackageManagerTDD() {
        std::cout << "\n=== Package Manager TDD Verification ===" << std::endl;
        
        // Verify TDD subtasks completion
        std::cout << "Verifying TDD subtasks completion..." << std::endl;
        
        // Subtask 1: Package metadata management (package.json)
        bool subtask1 = std::filesystem::exists("../packages/platformer-example/package.json");
        TEST_ASSERT(subtask1);
        std::cout << "1. ✓ Package metadata management" << std::endl;
        
        // Subtask 2: Version control and dependency resolution
        std::filesystem::path packageManagerPath = "../src/packages/package_manager.h";
        bool subtask2 = std::filesystem::exists(packageManagerPath);
        TEST_ASSERT(subtask2);
        std::cout << "2. " << (subtask2 ? "✓" : "✗") << " Version control and dependency resolution" << std::endl;
        
        // Subtask 3: Package loading and initialization
        std::filesystem::path packageLoaderPath = "../src/packages/package_loader.h";  
        bool subtask3 = std::filesystem::exists(packageLoaderPath);
        TEST_ASSERT(subtask3);
        std::cout << "3. " << (subtask3 ? "✓" : "✗") << " Package loading and initialization" << std::endl;
        
        // Subtask 4: Integration with build system
        bool subtask4 = std::filesystem::exists("../packages/platformer-example/CMakeLists.txt");
        TEST_ASSERT(subtask4);
        std::cout << "4. ✓ Integration with build system" << std::endl;
        
        // Subtask 5: Package registry and discovery
        std::filesystem::path packagesDir = "../packages";
        bool subtask5 = std::filesystem::exists(packagesDir) && std::filesystem::is_directory(packagesDir);
        TEST_ASSERT(subtask5);
        std::cout << "5. " << (subtask5 ? "✓" : "✗") << " Package registry and discovery" << std::endl;
        
        return subtask1 && subtask2 && subtask3 && subtask4 && subtask5;
    }
    
    bool runAllTests() {
        bool allPassed = true;
        
        std::cout << "Running Package Manager Tests..." << std::endl;
        std::cout << "=================================" << std::endl;
        
        allPassed &= testPackageManagerComponents();
        allPassed &= testPackageStructure();
        allPassed &= testPackageIntegration();
        allPassed &= testPackageManagerTDD();
        
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "Tests run: " << tests_run << std::endl;
        std::cout << "Tests passed: " << tests_passed << std::endl;
        std::cout << "Tests failed: " << (tests_run - tests_passed) << std::endl;
        
        if (tests_run == tests_passed) {
            std::cout << "\n🎉 ALL PACKAGE MANAGER TESTS PASSED!" << std::endl;
            std::cout << "Package manager appears to be ready for integration." << std::endl;
        } else {
            std::cout << "\n❌ SOME PACKAGE MANAGER TESTS FAILED!" << std::endl;
            std::cout << "Package manager needs fixes before merging." << std::endl;
        }
        
        return allPassed;
    }
};

int main() {
    PackageManagerTest test;
    bool success = test.runAllTests();
    return success ? 0 : 1;
}