#include <iostream>
#include <memory>
#include <sstream>
#include <regex>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include "../src/packages/package_manager.h"
#include "../src/console/console.h"
#include "../src/console/command_processor.h"
#include "../src/engine/command_registry.h"

// Test helper macros
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAIL: " << message << std::endl; \
        return 1; \
    }

// Mock console to capture output
class MockConsole : public Console {
public:
    struct LineEntry {
        std::string text;
        std::string colorName;
    };
    std::vector<LineEntry> lines;
    
    void addLine(const std::string& text, ::Color color = WHITE) {
        lines.push_back({text, colorToString(color)});
        Console::addLine(text, color);
    }
    
    void clear() {
        lines.clear();
    }
    
    bool hasLine(const std::string& pattern) const {
        std::regex re(pattern);
        for (const auto& line : lines) {
            if (std::regex_search(line.text, re)) {
                return true;
            }
        }
        return false;
    }
    
    void printLines() const {
        for (const auto& line : lines) {
            std::cout << "  [" << line.colorName << "] " << line.text << std::endl;
        }
    }
    
private:
    std::string colorToString(::Color color) const {
        // Compare color values
        if (color.r == 255 && color.g == 255 && color.b == 255) return "WHITE";
        if (color.r == 130 && color.g == 130 && color.b == 130) return "GRAY";
        if (color.r == 253 && color.g == 249 && color.b == 0) return "YELLOW";
        if (color.r == 0 && color.g == 228 && color.b == 48) return "GREEN";
        if (color.r == 230 && color.g == 41 && color.b == 55) return "RED";
        return "UNKNOWN";
    }
};

int main() {
    std::cout << "Running Package CLI tests..." << std::endl;
    
    // Setup test environment
    std::filesystem::path testDir = std::filesystem::temp_directory_path() / "test_cli";
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);
    
    // Create test packages
    auto createPackage = [&](const std::string& name, const std::string& version) {
        std::filesystem::create_directories(testDir / name);
        nlohmann::json packageJson;
        packageJson["name"] = name;
        packageJson["version"] = version;
        packageJson["description"] = "Test package " + name;
        packageJson["components"] = nlohmann::json::array();
        packageJson["systems"] = nlohmann::json::array();
        
        std::ofstream file(testDir / name / "package.json");
        file << packageJson.dump(2);
        file.close();
    };
    
    createPackage("test-pkg-1", "1.0.0");
    createPackage("test-pkg-2", "2.0.0");
    
    // Initialize systems
    auto packageManager = std::make_unique<GameEngine::PackageManager>(testDir);
    auto console = std::make_unique<MockConsole>();
    auto commandProcessor = std::make_unique<CommandProcessor>();
    auto commandRegistry = std::make_unique<GameEngine::CommandRegistry>();
    
    console->initialize();
    commandProcessor->initialize(console.get());
    console->setCommandProcessor(commandProcessor.get());
    
    // Register package commands using a minimal command registry approach
    // Since registerPackageCommands is private, we'll use registerAllCommands
    commandRegistry->registerAllCommands(
        commandProcessor.get(),
        nullptr,  // engineCore
        console.get(),
        []() -> Scene* { return nullptr; },  // getScene
        nullptr,  // resourceManager
        nullptr,  // scriptManager
        nullptr,  // gameLogicManager
        nullptr,  // projectManager
        nullptr,  // buildSystem
        nullptr,  // asyncBuildSystem
        nullptr,  // playMode
        nullptr,  // engine
        packageManager.get()
    );
    
    // Test 1: package.list command
    {
        std::cout << "\nTest 1: package.list command..." << std::endl;
        console->clear();
        
        commandProcessor->executeCommand("package.list");
        
        TEST_ASSERT(console->hasLine("Available packages:"), "Should show available packages header");
        TEST_ASSERT(console->hasLine("test-pkg-1"), "Should list test-pkg-1");
        TEST_ASSERT(console->hasLine("test-pkg-2"), "Should list test-pkg-2");
        
        std::cout << "PASS: package.list" << std::endl;
    }
    
    // Test 2: package.info command
    {
        std::cout << "\nTest 2: package.info command..." << std::endl;
        console->clear();
        
        commandProcessor->executeCommand("package.info test-pkg-1");
        
        TEST_ASSERT(console->hasLine("Package found but not loaded"), "Should indicate package not loaded");
        
        // Load package first
        packageManager->loadPackage("test-pkg-1");
        console->clear();
        
        commandProcessor->executeCommand("package.info test-pkg-1");
        
        TEST_ASSERT(console->hasLine("Package Information:"), "Should show package info header");
        TEST_ASSERT(console->hasLine("Name: test-pkg-1"), "Should show package name");
        TEST_ASSERT(console->hasLine("Version: 1.0.0"), "Should show package version");
        
        std::cout << "PASS: package.info" << std::endl;
    }
    
    // Test 3: package.load command
    {
        std::cout << "\nTest 3: package.load command..." << std::endl;
        console->clear();
        
        commandProcessor->executeCommand("package.load test-pkg-2");
        
        TEST_ASSERT(console->hasLine("Loading package: test-pkg-2"), "Should show loading message");
        TEST_ASSERT(console->hasLine("Package loaded successfully: test-pkg-2"), "Should show success message");
        
        // Try loading again
        console->clear();
        commandProcessor->executeCommand("package.load test-pkg-2");
        
        TEST_ASSERT(console->hasLine("Package already loaded: test-pkg-2"), "Should indicate already loaded");
        
        std::cout << "PASS: package.load" << std::endl;
    }
    
    // Test 4: package.loaded command
    {
        std::cout << "\nTest 4: package.loaded command..." << std::endl;
        console->clear();
        
        commandProcessor->executeCommand("package.loaded");
        
        TEST_ASSERT(console->hasLine("Loaded packages:"), "Should show loaded packages header");
        TEST_ASSERT(console->hasLine("test-pkg-1 v1.0.0"), "Should show test-pkg-1 with version");
        TEST_ASSERT(console->hasLine("test-pkg-2 v2.0.0"), "Should show test-pkg-2 with version");
        
        std::cout << "PASS: package.loaded" << std::endl;
    }
    
    // Test 5: package.refresh command
    {
        std::cout << "\nTest 5: package.refresh command..." << std::endl;
        
        // Add a new package
        createPackage("test-pkg-3", "3.0.0");
        
        console->clear();
        commandProcessor->executeCommand("package.refresh");
        
        TEST_ASSERT(console->hasLine("Scanning packages directory"), "Should show scanning message");
        TEST_ASSERT(console->hasLine("Found 3 packages"), "Should find 3 packages after refresh");
        
        std::cout << "PASS: package.refresh" << std::endl;
    }
    
    // Test 6: package.deps command with dependencies
    {
        std::cout << "\nTest 6: package.deps command..." << std::endl;
        
        // Create package with dependencies
        std::filesystem::create_directories(testDir / "pkg-with-deps");
        nlohmann::json packageJson;
        packageJson["name"] = "pkg-with-deps";
        packageJson["version"] = "1.0.0";
        packageJson["dependencies"] = nlohmann::json::object();
        packageJson["dependencies"]["test-pkg-1"] = ">=1.0.0";
        packageJson["dependencies"]["test-pkg-2"] = "^2.0.0";
        packageJson["components"] = nlohmann::json::array();
        packageJson["systems"] = nlohmann::json::array();
        
        std::ofstream file(testDir / "pkg-with-deps" / "package.json");
        file << packageJson.dump(2);
        file.close();
        
        packageManager->scanPackages();
        packageManager->loadPackage("pkg-with-deps");
        
        console->clear();
        commandProcessor->executeCommand("package.deps pkg-with-deps");
        
        TEST_ASSERT(console->hasLine("Dependencies for pkg-with-deps:"), "Should show dependencies header");
        TEST_ASSERT(console->hasLine("✓.*test-pkg-1.*>=1.0.0.*satisfied"), "Should show satisfied dependency");
        TEST_ASSERT(console->hasLine("✓.*test-pkg-2.*\\^2.0.0.*satisfied"), "Should show satisfied dependency");
        
        std::cout << "PASS: package.deps" << std::endl;
    }
    
    // Cleanup
    std::filesystem::remove_all(testDir);
    
    std::cout << "\nAll Package CLI tests passed!" << std::endl;
    return 0;
}