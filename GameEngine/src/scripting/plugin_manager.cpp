#include "plugin_manager.h"
#include "../utils/engine_paths.h"
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <dlfcn.h>

namespace GameEngine {
GameLogicPluginManager::GameLogicPluginManager() {
    // Add allowed plugin paths for security
    std::string engineRoot = EnginePaths::getEngineRoot();
    allowedPaths.insert(engineRoot + "/packages");
    allowedPaths.insert(engineRoot + "/build/packages");
    allowedPaths.insert("packages");
    allowedPaths.insert("./packages");
    allowedPaths.insert("build/packages");
    allowedPaths.insert("./build/packages");
}

GameLogicPluginManager::~GameLogicPluginManager() {
    clearAll();
}

void GameLogicPluginManager::registerGameLogicFactory(const std::string& name, std::function<std::unique_ptr<IGameLogic>()> factory) {
    gameLogicFactories[name] = factory;
    spdlog::info("PluginManager: Registered game logic factory: {}", name);
}

bool GameLogicPluginManager::loadPlugin(const std::string& path, const std::string& name) {
    // Security check: validate plugin path
    if (securityEnabled && !isPathAllowed(path)) {
        spdlog::error("PluginManager: Plugin path not allowed: {}", path);
        return false;
    }

    // Check if already loaded
    if (loadedLibraries.find(name) != loadedLibraries.end()) {
        spdlog::debug("PluginManager: Plugin already loaded: {}", name);
        return true;
    }

    // Check if file exists
    if (!std::filesystem::exists(path)) {
        spdlog::error("PluginManager: Plugin file not found: {}", path);
        return false;
    }

    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle) {
        spdlog::error("PluginManager: Failed to load plugin {}: {}", path, dlerror());
        return false;
    }

    // Verify plugin has required exports
    void* initFunc = dlsym(handle, "initializePlugin");
    if (!initFunc) {
        spdlog::error("PluginManager: Plugin {} missing required export: initializePlugin", name);
        dlclose(handle);
        return false;
    }

    // Initialize the plugin
    try {
        auto pluginInit = reinterpret_cast<void(*)(IPluginManager*)>(initFunc);
        pluginInit(this);

        loadedLibraries[name] = handle;
        spdlog::info("PluginManager: Loaded and initialized plugin: {}", name);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("PluginManager: Failed to initialize plugin {}: {}", name, e.what());
        dlclose(handle);
        return false;
    }
}

bool GameLogicPluginManager::unloadPlugin(const std::string& name) {
    auto it = loadedLibraries.find(name);
    if (it != loadedLibraries.end()) {
        dlclose(it->second);
        loadedLibraries.erase(it);
        spdlog::info("PluginManager: Unloaded plugin: {}", name);

        // Remove associated game logic factories
        auto factoryIt = gameLogicFactories.find(name);
        if (factoryIt != gameLogicFactories.end()) {
            gameLogicFactories.erase(factoryIt);
        }

        return true;
    }
    return false;
}

bool GameLogicPluginManager::loadProjectPlugins(const std::string& projectPath) {
    try {
        // Load project.json to get dependencies
        std::string projectFile = projectPath + "/project.json";
        if (!std::filesystem::exists(projectFile)) {
            spdlog::warn("PluginManager: Project file not found: {}", projectFile);
            return false;
        }

        std::ifstream file(projectFile);
        nlohmann::json projectData;
        file >> projectData;
        file.close();

        // Load dependencies
        if (projectData.contains("dependencies") && projectData["dependencies"].is_array()) {
            for (const auto& dep : projectData["dependencies"]) {
                std::string depName = dep.get<std::string>();
                if (!loadPackageFromProject(projectPath, depName)) {
                    spdlog::warn("PluginManager: Failed to load package dependency: {}", depName);
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        spdlog::error("PluginManager: Failed to load project plugins: {}", e.what());
        return false;
    }
}

bool GameLogicPluginManager::loadPackageFromProject(const std::string& projectPath, const std::string& packageName) {
    // Get engine root path
    std::string engineRoot = EnginePaths::getEngineRoot();

    // Try different possible package locations
    std::vector<std::string> possiblePaths = {
        engineRoot + "/packages/" + packageName,
        engineRoot + "/build/packages/" + packageName,
        projectPath + "/packages/" + packageName,
        projectPath + "/../packages/" + packageName,
        projectPath + "/../build/packages/" + packageName
    };

    for (const auto& packageDir : possiblePaths) {
        std::filesystem::path packagePath(packageDir);
        std::filesystem::path packageJson = packagePath / "package.json";

        spdlog::debug("PluginManager: Checking package path: {}", packageJson.string());

        if (!std::filesystem::exists(packageJson)) {
            continue;
        }

        spdlog::info("PluginManager: Found package.json at: {}", packageJson.string());

        try {
            // Read package.json
            std::ifstream packageFile(packageJson);
            nlohmann::json packageData;
            packageFile >> packageData;
            packageFile.close();

            // Load plugin if specified
            if (packageData.contains("plugin")) {
                const auto& plugin = packageData["plugin"];
                std::string library = plugin.value("library", "");

                if (!library.empty()) {
                    std::filesystem::path libraryPath = packagePath / library;
                    if (std::filesystem::exists(libraryPath)) {
                        return loadPlugin(libraryPath.string(), packageName);
                    }
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("PluginManager: Error loading package {}: {}", packageName, e.what());
        }
    }

    spdlog::warn("PluginManager: Package not found: {}", packageName);
    return false;
}

std::unique_ptr<IGameLogic> GameLogicPluginManager::createGameLogic(const std::string& name) {
    auto it = gameLogicFactories.find(name);
    if (it != gameLogicFactories.end()) {
        return it->second();
    }
    return nullptr;
}

void GameLogicPluginManager::disableSecurity() {
    securityEnabled = false;
    spdlog::warn("PluginManager: Plugin security disabled - use only for development!");
}

std::vector<std::string> GameLogicPluginManager::getLoadedPlugins() const {
    std::vector<std::string> plugins;
    for (const auto& [name, handle] : loadedLibraries) {
        plugins.push_back(name);
    }
    return plugins;
}

void GameLogicPluginManager::clearAll() {
    // Unload all libraries
    for (auto& [name, handle] : loadedLibraries) {
        if (handle) {
            dlclose(handle);
        }
    }
    loadedLibraries.clear();
    gameLogicFactories.clear();
}

bool GameLogicPluginManager::isPathAllowed(const std::string& path) {
    try {
        std::filesystem::path pluginPath(path);
        std::filesystem::path parentPath = pluginPath.parent_path();

        // If path doesn't exist yet, we can't canonicalize it
        if (!std::filesystem::exists(parentPath)) {
            return false;
        }

        std::filesystem::path canonicalPath = std::filesystem::canonical(parentPath);

        for (const auto& allowedPath : allowedPaths) {
            std::filesystem::path allowedCanonical;

            // Handle relative paths
            if (std::filesystem::exists(allowedPath)) {
                allowedCanonical = std::filesystem::canonical(allowedPath);
            } else {
                continue;
            }

            // Check if plugin path is within allowed directory
            auto relative = std::filesystem::relative(canonicalPath, allowedCanonical);
            if (!relative.empty() && relative.native()[0] != '.') {
                return true;
            }
        }

        return false;
    } catch (const std::exception& e) {
        spdlog::error("PluginManager: Error checking path permissions: {}", e.what());
        return false;
    }
}

} // namespace GameEngine
