#include "package_manager.h"
#include "package_loader.h"
#include "../plugins/plugin_manager.h"
#include "../utils/engine_paths.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <algorithm>
#include <functional>
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
        spdlog::warn("[PackageManager] Packages directory does not exist: {}", packagesPath.string());
        return;
    }
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(packagesPath)) {
            if (entry.is_directory()) {
                std::string packageName = entry.path().filename().string();
                
                // Skip hidden directories
                if (packageName.empty() || packageName[0] == '.') {
                    continue;
                }
                
                // Check if package.json exists
                auto packageJsonPath = entry.path() / "package.json";
                if (std::filesystem::exists(packageJsonPath)) {
                    availablePackages[packageName] = entry.path();
                    spdlog::debug("[PackageManager] Found package: {}", packageName);
                } else {
                    spdlog::debug("[PackageManager] Directory {} has no package.json, skipping", packageName);
                }
            }
        }
        
        spdlog::info("[PackageManager] Found {} packages", availablePackages.size());
    } catch (const std::exception& e) {
        spdlog::error("[PackageManager] Error scanning packages directory: {}", e.what());
    }
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
    if (packages.find(name) != packages.end() || loadedPackages.find(name) != loadedPackages.end()) {
        spdlog::warn("[PackageManager] Package {} is already loaded", name);
        return true;
    }
    
    auto packagePath = packagesPath / name;
    if (!std::filesystem::exists(packagePath)) {
        spdlog::error("[PackageManager] Package directory does not exist: {}", packagePath.string());
        return false;
    }
    
    return loadPackageMetadata(name, packagePath);
}

bool PackageManager::loadPackageWithDependencies(const std::string& name) {
    // First check dependencies
    auto resolution = checkDependencies(name);
    if (!resolution.satisfied) {
        lastError = "Dependencies not satisfied for package: " + name;
        spdlog::error("[PackageManager] {}", lastError);
        return false;
    }
    
    // Load packages in dependency order
    for (const auto& packageName : resolution.loadOrder) {
        if (!loadPackage(packageName)) {
            lastError = "Failed to load dependency: " + packageName;
            return false;
        }
    }
    
    return true;
}

bool PackageManager::unloadPackage(const std::string& name) {
    // TODO: Implement proper unloading
    auto it = packages.find(name);
    if (it != packages.end()) {
        packages.erase(it);
        spdlog::info("[PackageManager] Unloaded package: {}", name);
        return true;
    }
    
    auto it2 = loadedPackages.find(name);
    if (it2 != loadedPackages.end()) {
        loadedPackages.erase(it2);
        spdlog::info("[PackageManager] Unloaded package: {}", name);
        return true;
    }
    
    return false;
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

std::shared_ptr<Package> PackageManager::getPackageShared(const std::string& packageName) const {
    auto it = loadedPackages.find(packageName);
    if (it != loadedPackages.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> PackageManager::getLoadedPackages() const {
    std::vector<std::string> result;
    for (const auto& [name, package] : packages) {
        result.push_back(name);
    }
    for (const auto& [name, package] : loadedPackages) {
        if (std::find(result.begin(), result.end(), name) == result.end()) {
            result.push_back(name);
        }
    }
    return result;
}

std::optional<PackageInfo> PackageManager::getPackageInfo(const std::string& packageName) const {
    auto package = getPackage(packageName);
    if (package) {
        return PackageInfo{
            package->getName(),
            package->getVersion(),
            package->getDescription()
        };
    }
    
    auto sharedPackage = getPackageShared(packageName);
    if (sharedPackage) {
        return PackageInfo{
            sharedPackage->getName(),
            sharedPackage->getVersion(),
            sharedPackage->getDescription()
        };
    }
    
    return std::nullopt;
}

bool PackageManager::loadPackageMetadata(const std::string& name, const std::filesystem::path& packagePath) {
    auto packageJsonPath = packagePath / "package.json";
    
    if (!std::filesystem::exists(packageJsonPath)) {
        lastError = "package.json not found for package: " + name;
        spdlog::error("[PackageManager] {}", lastError);
        return false;
    }
    
    try {
        // Read package.json
        std::ifstream file(packageJsonPath);
        if (!file.is_open()) {
            lastError = "Failed to open package.json for: " + name;
            spdlog::error("[PackageManager] {}", lastError);
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        file.close();
        
        // Create package object
        std::string version = j.value("version", "0.0.0");
        auto package = std::make_unique<Package>(name, version);
        
        // Set metadata
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
        
        // Load plugin info if present
        if (j.contains("plugin")) {
            PackagePluginInfo pluginInfo;
            pluginInfo.library = j["plugin"].value("library", "");
            pluginInfo.main = j["plugin"].value("main", "");
            pluginInfo.autoload = j["plugin"].value("autoload", true);
            if (!pluginInfo.library.empty()) {
                package->setPluginInfo(pluginInfo);
            }
        }
        
        // Store the loaded package
        packages[name] = std::move(package);
        
        // Also create shared version for compatibility
        auto sharedPackage = std::make_shared<Package>(name, version);
        sharedPackage->setDescription(j.value("description", ""));
        sharedPackage->setAuthor(j.value("author", ""));
        sharedPackage->setLicense(j.value("license", ""));
        sharedPackage->setEngineVersion(j.value("engineVersion", ""));
        
        if (j.contains("dependencies")) {
            for (auto& [depName, depVersion] : j["dependencies"].items()) {
                sharedPackage->addDependency(depName, depVersion);
            }
        }
        
        if (j.contains("components")) {
            for (const auto& comp : j["components"]) {
                ComponentInfo info;
                info.name = comp["name"];
                info.file = comp.value("file", "");
                sharedPackage->addComponent(info);
            }
        }
        
        if (j.contains("systems")) {
            for (const auto& sys : j["systems"]) {
                SystemInfo info;
                info.name = sys["name"];
                info.file = sys.value("file", "");
                info.priority = sys.value("priority", 0);
                sharedPackage->addSystem(info);
            }
        }
        
        loadedPackages[name] = sharedPackage;
        
        // Load package resources using appropriate loader
        PackageLoader* loader = packageLoader ? packageLoader : &internalPackageLoader;
        if (!loader->loadPackageResources(*sharedPackage, packagePath)) {
            spdlog::error("[PackageManager] Failed to load resources for package: {}", name);
            packages.erase(name);
            loadedPackages.erase(name);
            return false;
        }
        
        spdlog::info("[PackageManager] Loaded package: {} v{}", name, version);
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
        // Try to find in available packages
        auto availIt = availablePackages.find(packageName);
        if (availIt == availablePackages.end()) {
            result.satisfied = false;
            result.missing.push_back(packageName);
            return result;
        }
    }
    
    // For now, simplified dependency checking
    result.satisfied = true;
    result.loadOrder.push_back(packageName);
    
    return result;
}

bool PackageManager::isVersionCompatible(const std::string& required, const std::string& actual) const {
    if (required.empty()) return true;
    
    std::string op, version;
    if (!parseVersionRequirement(required, op, version)) {
        // If no operator, assume exact match
        return required == actual;
    }
    
    int cmp = compareVersions(actual, version);
    
    if (op == ">=") return cmp >= 0;
    if (op == ">") return cmp > 0;
    if (op == "<=") return cmp <= 0;
    if (op == "<") return cmp < 0;
    if (op == "==" || op == "=") return cmp == 0;
    if (op == "^") {
        // Caret - compatible with version
        auto actualParts = splitVersion(actual);
        auto requiredParts = splitVersion(version);
        if (actualParts.empty() || requiredParts.empty()) return false;
        
        // Must be at least the required version
        if (cmp < 0) return false;
        
        // Check compatibility based on major version
        if (requiredParts[0] == 0) {
            // Special handling for 0.x.x versions
            if (requiredParts.size() > 1 && requiredParts[1] == 0) {
                // 0.0.x - only patch updates allowed
                return actualParts.size() >= 2 && 
                       actualParts[0] == 0 && 
                       actualParts[1] == 0;
            } else {
                // 0.x.y - minor and patch updates allowed within same minor
                return actualParts[0] == 0 && 
                       actualParts.size() > 1 && 
                       requiredParts.size() > 1 &&
                       actualParts[1] == requiredParts[1];
            }
        } else {
            // Normal case: major version must match
            return actualParts[0] == requiredParts[0];
        }
    }
    
    return false;
}

bool PackageManager::checkEngineCompatibility(const Package& package) const {
    std::string engineReq = package.getEngineVersion();
    if (engineReq.empty()) return true;
    
    return isVersionCompatible(engineReq, currentEngineVersion);
}

std::vector<ComponentInfo> PackageManager::getAllComponents() const {
    std::vector<ComponentInfo> allComponents;
    
    for (const auto& [name, package] : packages) {
        const auto& components = package->getComponents();
        allComponents.insert(allComponents.end(), components.begin(), components.end());
    }
    
    for (const auto& [name, package] : loadedPackages) {
        const auto& components = package->getComponents();
        allComponents.insert(allComponents.end(), components.begin(), components.end());
    }
    
    return allComponents;
}

std::vector<SystemInfo> PackageManager::getAllSystems() const {
    std::vector<SystemInfo> allSystems;
    
    for (const auto& [name, package] : packages) {
        const auto& systems = package->getSystems();
        allSystems.insert(allSystems.end(), systems.begin(), systems.end());
    }
    
    for (const auto& [name, package] : loadedPackages) {
        const auto& systems = package->getSystems();
        allSystems.insert(allSystems.end(), systems.begin(), systems.end());
    }
    
    // Sort by priority (higher priority first)
    std::sort(allSystems.begin(), allSystems.end(), 
              [](const SystemInfo& a, const SystemInfo& b) {
                  return a.priority > b.priority;
              });
    
    return allSystems;
}

bool PackageManager::parseVersionRequirement(const std::string& requirement, std::string& op, std::string& version) const {
    static const std::vector<std::string> operators = {">=", "<=", ">", "<", "==", "=", "^"};
    
    for (const auto& oper : operators) {
        if (requirement.find(oper) == 0) {
            op = oper;
            version = requirement.substr(oper.length());
            // Trim whitespace
            version.erase(0, version.find_first_not_of(" \t"));
            return true;
        }
    }
    
    return false;
}

int PackageManager::compareVersions(const std::string& v1, const std::string& v2) const {
    auto parts1 = splitVersion(v1);
    auto parts2 = splitVersion(v2);
    
    size_t maxSize = std::max(parts1.size(), parts2.size());
    parts1.resize(maxSize, 0);
    parts2.resize(maxSize, 0);
    
    for (size_t i = 0; i < maxSize; ++i) {
        if (parts1[i] < parts2[i]) return -1;
        if (parts1[i] > parts2[i]) return 1;
    }
    
    return 0;
}

std::vector<int> PackageManager::splitVersion(const std::string& version) const {
    std::vector<int> parts;
    std::stringstream ss(version);
    std::string part;
    
    while (std::getline(ss, part, '.')) {
        try {
            parts.push_back(std::stoi(part));
        } catch (...) {
            parts.push_back(0);
        }
    }
    
    return parts;
}

bool PackageManager::hasCircularDependency(const std::string& packageName) const {
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursionStack;
    
    std::function<bool(const std::string&)> dfs = [&](const std::string& pkg) -> bool {
        if (recursionStack.count(pkg)) {
            return true; // Circular dependency found
        }
        if (visited.count(pkg)) {
            return false; // Already processed
        }
        
        visited.insert(pkg);
        recursionStack.insert(pkg);
        
        // Check package dependencies
        auto package = getPackage(pkg);
        if (package) {
            for (const auto& dep : package->getDependencies()) {
                if (dfs(dep.name)) {
                    return true;
                }
            }
        }
        
        recursionStack.erase(pkg);
        return false;
    };
    
    return dfs(packageName);
}

std::vector<std::string> PackageManager::getDependencyOrder(const std::string& packageName) const {
    std::vector<std::string> result;
    std::unordered_set<std::string> visited;
    
    std::function<void(const std::string&)> topologicalSort = [&](const std::string& pkg) {
        if (visited.count(pkg)) {
            return;
        }
        
        visited.insert(pkg);
        
        // First, recursively visit all dependencies
        auto package = getPackage(pkg);
        if (package) {
            for (const auto& dep : package->getDependencies()) {
                topologicalSort(dep.name);
            }
        }
        
        // Then add current package to result
        result.push_back(pkg);
    };
    
    topologicalSort(packageName);
    return result;
}

} // namespace GameEngine