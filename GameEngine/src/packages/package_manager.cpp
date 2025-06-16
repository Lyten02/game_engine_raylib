#include "package_manager.h"
#include "package_loader.h"
#include "../plugins/plugin_manager.h"
#include "../utils/engine_paths.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_set>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace GameEngine {

PackageManager::PackageManager(const std::filesystem::path& packagesPath)
    : packagesPath(packagesPath) {
    spdlog::info("[PackageManager] Initialized with path: {}", packagesPath.string());
}

PackageManager::~PackageManager() = default;

void PackageManager::scanPackages() {
    availablePackages.clear();
    
    if (!std::filesystem::exists(packagesPath)) {
        spdlog::error("[PackageManager] Packages directory does not exist: {}", packagesPath.string());
        return;
    }
    
    spdlog::debug("[PackageManager] Scanning packages directory: {}", packagesPath.string());
    
    for (const auto& entry : std::filesystem::directory_iterator(packagesPath)) {
        if (!entry.is_directory()) continue;
        
        auto packageJsonPath = entry.path() / "package.json";
        if (std::filesystem::exists(packageJsonPath)) {
            std::string packageName = entry.path().filename().string();
            availablePackages[packageName] = entry.path();
            spdlog::debug("[PackageManager] Found package: {}", packageName);
        }
    }
    
    spdlog::info("[PackageManager] Found {} packages", availablePackages.size());
}

std::vector<std::string> PackageManager::getAvailablePackages() const {
    std::vector<std::string> result;
    for (const auto& [name, path] : availablePackages) {
        result.push_back(name);
    }
    return result;
}

bool PackageManager::loadPackage(const std::string& name) {
    // Check if already loaded
    if (packages.find(name) != packages.end()) {
        spdlog::warn("[PackageManager] Package already loaded: {}", name);
        return true;
    }
    
    // Find package path
    auto it = availablePackages.find(name);
    if (it == availablePackages.end()) {
        lastError = "Package not found: " + name;
        spdlog::error("[PackageManager] {}", lastError);
        return false;
    }
    
    // Load package metadata
    if (!loadPackageMetadata(name, it->second)) {
        return false;
    }
    
    // Load package resources if loader is available
    if (packageLoader) {
        auto package = packages[name].get();
        if (!packageLoader->loadPackageResources(*package, it->second)) {
            lastError = "Failed to load package resources: " + packageLoader->getLastError();
            spdlog::error("[PackageManager] {}", lastError);
            packages.erase(name);
            return false;
        }
    }
    
    spdlog::info("[PackageManager] Successfully loaded package: {}", name);
    return true;
}

bool PackageManager::loadPackageWithDependencies(const std::string& name) {
    // First check if we can resolve all dependencies
    auto resolution = checkDependencies(name);
    if (!resolution.satisfied) {
        lastError = "Dependency resolution failed for package: " + name;
        spdlog::error("[PackageManager] {}", lastError);
        return false;
    }
    
    // Load packages in dependency order
    for (const auto& pkgName : resolution.loadOrder) {
        if (!loadPackage(pkgName)) {
            return false;
        }
    }
    
    return true;
}

bool PackageManager::unloadPackage(const std::string& name) {
    auto it = packages.find(name);
    if (it == packages.end()) {
        lastError = "Package not loaded: " + name;
        spdlog::error("[PackageManager] {}", lastError);
        return false;
    }
    
    // TODO: Properly unload package resources, components, systems, and plugins
    spdlog::warn("[PackageManager] Package unloading not fully implemented yet");
    
    packages.erase(it);
    spdlog::info("[PackageManager] Unloaded package: {}", name);
    return true;
}

Package* PackageManager::getPackage(const std::string& name) {
    auto it = packages.find(name);
    if (it != packages.end()) {
        return it->second.get();
    }
    return nullptr;
}

const Package* PackageManager::getPackage(const std::string& name) const {
    auto it = packages.find(name);
    if (it != packages.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> PackageManager::getLoadedPackages() const {
    std::vector<std::string> result;
    for (const auto& [name, package] : packages) {
        result.push_back(name);
    }
    return result;
}

bool PackageManager::loadPackageMetadata(const std::string& name, const std::filesystem::path& packagePath) {
    auto packageJsonPath = packagePath / "package.json";
    
    std::ifstream file(packageJsonPath);
    if (!file.is_open()) {
        lastError = "Failed to open package.json for: " + name;
        spdlog::error("[PackageManager] {}", lastError);
        return false;
    }
    
    try {
        nlohmann::json j;
        file >> j;
        file.close();
        
        // Create package object
        std::string version = j.value("version", "0.0.0");
        auto package = std::make_unique<Package>(name, version);
        
        // Set basic metadata
        package->setDescription(j.value("description", ""));
        package->setAuthor(j.value("author", ""));
        package->setLicense(j.value("license", ""));
        package->setEngineVersion(j.value("engineVersion", ""));
        
        // Load dependencies
        if (j.contains("dependencies")) {
            for (auto& [depName, depVersion] : j["dependencies"].items()) {
                package->addDependency(depName, depVersion);
            }
        }
        
        // Load components
        if (j.contains("components")) {
            for (const auto& comp : j["components"]) {
                ComponentInfo info;
                info.name = comp["name"];
                info.file = comp.value("file", "");
                package->addComponent(info);
            }
        }
        
        // Load systems
        if (j.contains("systems")) {
            for (const auto& sys : j["systems"]) {
                SystemInfo info;
                info.name = sys["name"];
                info.file = sys.value("file", "");
                info.priority = sys.value("priority", 0);
                package->addSystem(info);
            }
        }
        
        // Load plugin info
        if (j.contains("plugin")) {
            PackagePluginInfo pluginInfo;
            pluginInfo.library = j["plugin"]["library"];
            pluginInfo.main = j["plugin"].value("main", "");
            pluginInfo.autoload = j["plugin"].value("autoload", true);
            package->setPluginInfo(pluginInfo);
        }
        
        packages[name] = std::move(package);
        return true;
        
    } catch (const std::exception& e) {
        lastError = "Failed to parse package.json for " + name + ": " + e.what();
        spdlog::error("[PackageManager] {}", lastError);
        return false;
    }
}

DependencyResolution PackageManager::checkDependencies(const std::string& packageName) const {
    DependencyResolution result;
    
    // First, ensure we have metadata for the package
    Package* package = nullptr;
    auto it = packages.find(packageName);
    if (it != packages.end()) {
        package = it->second.get();
    } else {
        // Try to load metadata temporarily
        auto availIt = availablePackages.find(packageName);
        if (availIt == availablePackages.end()) {
            result.satisfied = false;
            result.missing.push_back(packageName);
            return result;
        }
    }
    
    if (package) {
        // Check each dependency
        for (const auto& dep : package->getDependencies()) {
            // Check if dependency exists
            auto depIt = availablePackages.find(dep.name);
            if (depIt == availablePackages.end()) {
                result.satisfied = false;
                result.missing.push_back(dep.name);
                continue;
            }
            
            // Check version compatibility if loaded
            auto loadedDep = packages.find(dep.name);
            if (loadedDep != packages.end()) {
                if (!dep.version.empty() && 
                    !isVersionCompatible(dep.version, loadedDep->second->getVersion())) {
                    result.satisfied = false;
                    result.incompatible.push_back(dep.name + " requires " + dep.version + 
                                                " but " + loadedDep->second->getVersion() + " is loaded");
                }
            }
        }
    }
    
    // Build load order if satisfied
    if (result.satisfied) {
        result.loadOrder = getDependencyOrder(packageName);
    }
    
    return result;
}

std::vector<std::string> PackageManager::getDependencyOrder(const std::string& packageName) const {
    std::vector<std::string> order;
    std::unordered_set<std::string> visited;
    
    buildDependencyOrder(packageName, visited, order);
    
    return order;
}

void PackageManager::buildDependencyOrder(const std::string& packageName,
                                        std::unordered_set<std::string>& visited,
                                        std::vector<std::string>& order) const {
    if (visited.find(packageName) != visited.end()) {
        return;
    }
    
    visited.insert(packageName);
    
    // First, visit all dependencies
    auto it = packages.find(packageName);
    if (it != packages.end()) {
        for (const auto& dep : it->second->getDependencies()) {
            buildDependencyOrder(dep.name, visited, order);
        }
    }
    
    // Then add this package
    order.push_back(packageName);
}

bool PackageManager::hasCircularDependency(const std::string& packageName) const {
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursionStack;
    
    return checkCircularDependencyRecursive(packageName, visited, recursionStack);
}

bool PackageManager::checkCircularDependencyRecursive(const std::string& packageName,
                                                    std::unordered_set<std::string>& visited,
                                                    std::unordered_set<std::string>& recursionStack) const {
    visited.insert(packageName);
    recursionStack.insert(packageName);
    
    auto it = packages.find(packageName);
    if (it != packages.end()) {
        for (const auto& dep : it->second->getDependencies()) {
            if (recursionStack.find(dep.name) != recursionStack.end()) {
                return true; // Found circular dependency
            }
            
            if (visited.find(dep.name) == visited.end() &&
                checkCircularDependencyRecursive(dep.name, visited, recursionStack)) {
                return true;
            }
        }
    }
    
    recursionStack.erase(packageName);
    return false;
}

bool PackageManager::isVersionCompatible(const std::string& requirement, const std::string& version) const {
    // Simple version compatibility check
    // TODO: Implement proper semantic versioning
    
    if (requirement.empty() || version.empty()) {
        return true;
    }
    
    // Handle basic operators
    if (requirement[0] == '^' || requirement[0] == '~') {
        // For now, just check major version
        std::string reqVersion = requirement.substr(1);
        return version[0] == reqVersion[0]; // Simple major version check
    }
    
    if (requirement.substr(0, 2) == ">=") {
        // For now, just return true
        return true;
    }
    
    // Exact match
    return requirement == version;
}

} // namespace GameEngine