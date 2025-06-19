#include <iostream>
#include <memory>
#include <cassert>
#include "../src/build/build_system.h"
#include "../src/build/async_build_system.h"
#include "../src/build/build_config.h"
#include "../src/project/project.h"
#include <filesystem>

using namespace GameEngine;

void testBuildSystemCreation() {
    std::cout << "Testing BuildSystem creation..." << std::endl;
    
    auto buildSystem = std::make_unique<BuildSystem>();
    assert(buildSystem != nullptr);
    
    std::cout << "✓ BuildSystem created successfully" << std::endl;
}

void testAsyncBuildSystemCreation() {
    std::cout << "Testing AsyncBuildSystem creation..." << std::endl;
    
    auto asyncBuildSystem = std::make_unique<AsyncBuildSystem>();
    assert(asyncBuildSystem != nullptr);
    
    // Check initial status
    assert(asyncBuildSystem->getStatus() == AsyncBuildSystem::BuildStatus::Idle);
    assert(asyncBuildSystem->getProgress() == 0.0f);
    
    std::cout << "✓ AsyncBuildSystem created successfully" << std::endl;
    std::cout << "✓ Initial status is Idle" << std::endl;
    std::cout << "✓ Initial progress is 0%" << std::endl;
}

void testBuildConfigCreation() {
    std::cout << "Testing BuildConfig creation..." << std::endl;
    
    // Test default config
    BuildConfig defaultConfig;
    assert(defaultConfig.getBuildType() == BuildConfig::BuildType::Release);
    std::cout << "✓ Default BuildConfig created (Release)" << std::endl;
    
    // Test debug config
    BuildConfig debugConfig(BuildConfig::BuildType::Debug);
    assert(debugConfig.getBuildType() == BuildConfig::BuildType::Debug);
    std::cout << "✓ Debug BuildConfig created" << std::endl;
    
    // Test static factory methods
    auto defaultDebug = BuildConfig::getDefaultDebugConfig();
    assert(defaultDebug.getBuildType() == BuildConfig::BuildType::Debug);
    std::cout << "✓ Default debug config created via factory" << std::endl;
    
    auto defaultRelease = BuildConfig::getDefaultReleaseConfig();
    assert(defaultRelease.getBuildType() == BuildConfig::BuildType::Release);
    std::cout << "✓ Default release config created via factory" << std::endl;
}

void testBuildDirectoryCreation() {
    std::cout << "Testing build directory creation..." << std::endl;
    
    auto buildSystem = std::make_unique<BuildSystem>();
    
    // Create a test directory
    std::string testProjectName = "TestBuildSystemProject_" + std::to_string(time(nullptr));
    bool result = buildSystem->createBuildDirectory(testProjectName);
    
    // Check if directory was created
    std::string outputDir = "output/" + testProjectName;
    bool dirExists = std::filesystem::exists(outputDir);
    
    assert(result == true);
    assert(dirExists == true);
    
    // Clean up
    if (dirExists) {
        std::filesystem::remove_all(outputDir);
    }
    
    std::cout << "✓ Build directory created successfully" << std::endl;
}

void testBuildConfigJsonSerialization() {
    std::cout << "Testing BuildConfig JSON serialization..." << std::endl;
    
    BuildConfig config(BuildConfig::BuildType::Debug);
    config.getCompilerOptions().flags.push_back("-Wall");
    config.getCompilerOptions().defines.push_back("DEBUG");
    
    // Serialize to JSON
    auto json = config.toJson();
    
    // Create new config from JSON
    BuildConfig config2;
    config2.fromJson(json);
    
    // Verify they match
    assert(config.getBuildType() == config2.getBuildType());
    assert(config.getCompilerOptions().flags.size() == config2.getCompilerOptions().flags.size());
    assert(config.getCompilerOptions().defines.size() == config2.getCompilerOptions().defines.size());
    
    std::cout << "✓ BuildConfig JSON serialization works" << std::endl;
}

int main() {
    std::cout << "=== Build System Basic Tests ===" << std::endl;
    
    try {
        testBuildSystemCreation();
        testAsyncBuildSystemCreation();
        testBuildConfigCreation();
        testBuildDirectoryCreation();
        testBuildConfigJsonSerialization();
        
        std::cout << "\nAll tests passed! ✓" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}