#include <iostream>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "../src/packages/package_loader.h"
#include "../src/plugins/plugin_manager.h"

// Test helper macros
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAIL: " << message << std::endl; \
        return 1; \
    }

// Extended PackageLoader for plugin support
class PluginAwarePackageLoader : public GameEngine::PackageLoader {
public:
    PluginAwarePackageLoader(GameEngine::PluginManager* pluginMgr) 
        : pluginManager(pluginMgr) {}
    
    bool loadPackagePlugin(const std::filesystem::path& packagePath) {
        // Look for package.json
        auto packageJsonPath = packagePath / "package.json";
        if (!std::filesystem::exists(packageJsonPath)) {
            return false;
        }
        
        // Parse package.json
        std::ifstream file(packageJsonPath);
        nlohmann::json packageJson;
        file >> packageJson;
        file.close();
        
        // Check for plugin field
        if (!packageJson.contains("plugin")) {
            return true; // No plugin, that's OK
        }
        
        auto pluginInfo = packageJson["plugin"];
        if (!pluginInfo.contains("library")) {
            lastError = "Plugin info missing 'library' field";
            return false;
        }
        
        // Build plugin path
        std::string libraryName = pluginInfo["library"];
        auto pluginPath = packagePath / libraryName;
        
        // Load the plugin
        if (!pluginManager->loadPlugin(pluginPath)) {
            lastError = "Failed to load plugin: " + pluginManager->getLastError();
            return false;
        }
        
        return true;
    }
    
    std::string getLastError() const { return lastError; }
    
private:
    GameEngine::PluginManager* pluginManager;
    std::string lastError;
};

int main() {
    std::cout << "Running package plugin integration tests..." << std::endl;
    
    // Setup test directory
    std::filesystem::path testDir = std::filesystem::temp_directory_path() / "test_plugin_integration";
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);
    
    // Test 1: Package without plugin
    {
        std::cout << "\nTest 1: Package without plugin..." << std::endl;
        
        // Create package directory
        auto packageDir = testDir / "simple-package";
        std::filesystem::create_directories(packageDir);
        
        // Create package.json without plugin
        nlohmann::json packageJson;
        packageJson["name"] = "simple-package";
        packageJson["version"] = "1.0.0";
        packageJson["components"] = nlohmann::json::array();
        packageJson["systems"] = nlohmann::json::array();
        
        std::ofstream file(packageDir / "package.json");
        file << packageJson.dump(2);
        file.close();
        
        // Test loading
        GameEngine::PluginManager pluginManager(nullptr);
        PluginAwarePackageLoader loader(&pluginManager);
        
        bool result = loader.loadPackagePlugin(packageDir);
        TEST_ASSERT(result == true, "Should load package without plugin");
        TEST_ASSERT(pluginManager.getLoadedPlugins().empty(), "Should have no plugins loaded");
        
        std::cout << "PASS: Package without plugin" << std::endl;
    }
    
    // Test 2: Package with plugin field
    {
        std::cout << "\nTest 2: Package with plugin field..." << std::endl;
        
        // Create package directory
        auto packageDir = testDir / "plugin-package";
        std::filesystem::create_directories(packageDir);
        
        // Create package.json with plugin
        nlohmann::json packageJson;
        packageJson["name"] = "plugin-package";
        packageJson["version"] = "1.0.0";
        packageJson["plugin"] = {
            {"library", "myplugin.so"},
            {"main", "MyPlugin"}
        };
        
        std::ofstream file(packageDir / "package.json");
        file << packageJson.dump(2);
        file.close();
        
        // Test loading (will fail because plugin doesn't exist)
        GameEngine::PluginManager pluginManager(nullptr);
        PluginAwarePackageLoader loader(&pluginManager);
        
        bool result = loader.loadPackagePlugin(packageDir);
        TEST_ASSERT(result == false, "Should fail to load non-existent plugin");
        TEST_ASSERT(!loader.getLastError().empty(), "Should have error message");
        
        std::cout << "PASS: Package with plugin field" << std::endl;
    }
    
    // Test 3: Plugin info validation
    {
        std::cout << "\nTest 3: Plugin info validation..." << std::endl;
        
        // Create package with invalid plugin info
        auto packageDir = testDir / "invalid-plugin-package";
        std::filesystem::create_directories(packageDir);
        
        nlohmann::json packageJson;
        packageJson["name"] = "invalid-plugin-package";
        packageJson["version"] = "1.0.0";
        packageJson["plugin"] = {
            // Missing 'library' field
            {"main", "MyPlugin"}
        };
        
        std::ofstream file(packageDir / "package.json");
        file << packageJson.dump(2);
        file.close();
        
        // Test loading
        GameEngine::PluginManager pluginManager(nullptr);
        PluginAwarePackageLoader loader(&pluginManager);
        
        bool result = loader.loadPackagePlugin(packageDir);
        TEST_ASSERT(result == false, "Should fail with missing library field");
        TEST_ASSERT(loader.getLastError().find("missing 'library' field") != std::string::npos, 
                   "Should have specific error about missing field");
        
        std::cout << "PASS: Plugin info validation" << std::endl;
    }
    
    // Test 4: Multiple package loading
    {
        std::cout << "\nTest 4: Multiple package loading..." << std::endl;
        
        GameEngine::PluginManager pluginManager(nullptr);
        PluginAwarePackageLoader loader(&pluginManager);
        
        // Create multiple packages
        for (int i = 1; i <= 3; i++) {
            auto packageDir = testDir / ("package" + std::to_string(i));
            std::filesystem::create_directories(packageDir);
            
            nlohmann::json packageJson;
            packageJson["name"] = "package" + std::to_string(i);
            packageJson["version"] = "1.0.0";
            
            if (i == 2) {
                // Only package 2 has a plugin
                packageJson["plugin"] = {
                    {"library", "plugin2.so"}
                };
            }
            
            std::ofstream file(packageDir / "package.json");
            file << packageJson.dump(2);
            file.close();
        }
        
        // Load all packages
        bool result1 = loader.loadPackagePlugin(testDir / "package1");
        bool result2 = loader.loadPackagePlugin(testDir / "package2");
        bool result3 = loader.loadPackagePlugin(testDir / "package3");
        
        TEST_ASSERT(result1 == true, "Package 1 should load");
        TEST_ASSERT(result2 == false, "Package 2 should fail (plugin doesn't exist)");
        TEST_ASSERT(result3 == true, "Package 3 should load");
        
        std::cout << "PASS: Multiple package loading" << std::endl;
    }
    
    // Cleanup
    std::filesystem::remove_all(testDir);
    
    std::cout << "\nAll package plugin integration tests passed!" << std::endl;
    return 0;
}