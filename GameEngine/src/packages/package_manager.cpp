#include "package_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace GameEngine {

PackageManager::PackageManager(const std::filesystem::path& packagesDir)
    : packagesDirectory(packagesDir) {
}

void PackageManager::scanPackages() {
    availablePackages.clear();
    
    if (!std::filesystem::exists(packagesDirectory)) {
        spdlog::warn("[PackageManager] Packages directory does not exist: {}", packagesDirectory.string());
        return;
    }
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(packagesDirectory)) {
            if (entry.is_directory()) {
                std::string packageName = entry.path().filename().string();
                
                // Skip hidden directories
                if (packageName.empty() || packageName[0] == '.') {
                    continue;
                }
                
                // Check if package.json exists
                auto packageJsonPath = entry.path() / "package.json";
                if (std::filesystem::exists(packageJsonPath)) {
                    availablePackages.push_back(packageName);
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
    return availablePackages;
}

bool PackageManager::loadPackage(const std::string& packageName) {
    // Check if already loaded
    if (loadedPackages.find(packageName) != loadedPackages.end()) {
        spdlog::warn("[PackageManager] Package {} is already loaded", packageName);
        return true;
    }
    
    auto packagePath = packagesDirectory / packageName;
    if (!std::filesystem::exists(packagePath)) {
        spdlog::error("[PackageManager] Package directory does not exist: {}", packagePath.string());
        return false;
    }
    
    return loadPackageMetadata(packageName, packagePath);
}

bool PackageManager::unloadPackage(const std::string& packageName) {
    // TODO: Implement unloading
    return false;
}

std::shared_ptr<Package> PackageManager::getPackage(const std::string& packageName) const {
    auto it = loadedPackages.find(packageName);
    if (it != loadedPackages.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> PackageManager::getLoadedPackages() const {
    std::vector<std::string> result;
    for (const auto& [name, package] : loadedPackages) {
        result.push_back(name);
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
    return std::nullopt;
}

bool PackageManager::loadPackageMetadata(const std::string& packageName, const std::filesystem::path& packagePath) {
    auto packageJsonPath = packagePath / "package.json";
    
    if (!std::filesystem::exists(packageJsonPath)) {
        spdlog::error("[PackageManager] package.json not found for package: {}", packageName);
        return false;
    }
    
    try {
        // Read package.json
        std::ifstream file(packageJsonPath);
        if (!file.is_open()) {
            spdlog::error("[PackageManager] Failed to open package.json: {}", packageJsonPath.string());
            return false;
        }
        
        nlohmann::json packageJson;
        file >> packageJson;
        file.close();
        
        // Extract package information
        std::string name = packageJson.value("name", packageName);
        std::string version = packageJson.value("version", "1.0.0");
        std::string description = packageJson.value("description", "");
        std::string author = packageJson.value("author", "");
        std::string license = packageJson.value("license", "");
        std::string engineVersion = packageJson.value("engineVersion", "");
        
        // Create Package object
        auto package = std::make_shared<Package>(name, version);
        package->setDescription(description);
        package->setAuthor(author);
        package->setLicense(license);
        package->setEngineVersion(engineVersion);
        
        // Load dependencies if present
        if (packageJson.contains("dependencies")) {
            for (auto& [depName, depVersion] : packageJson["dependencies"].items()) {
                package->addDependency(depName, depVersion);
            }
        }
        
        // Load components if present
        if (packageJson.contains("components")) {
            for (const auto& compJson : packageJson["components"]) {
                ComponentInfo component;
                component.name = compJson.value("name", "");
                component.file = compJson.value("file", "");
                if (!component.name.empty() && !component.file.empty()) {
                    package->addComponent(component);
                }
            }
        }
        
        // Load systems if present
        if (packageJson.contains("systems")) {
            for (const auto& sysJson : packageJson["systems"]) {
                SystemInfo system;
                system.name = sysJson.value("name", "");
                system.file = sysJson.value("file", "");
                system.priority = sysJson.value("priority", 0);
                if (!system.name.empty() && !system.file.empty()) {
                    package->addSystem(system);
                }
            }
        }
        
        // Store the loaded package
        loadedPackages[packageName] = package;
        
        // Load package resources (components and systems)
        if (!packageLoader.loadPackageResources(*package, packagePath)) {
            spdlog::error("[PackageManager] Failed to load resources for package: {}", packageName);
            loadedPackages.erase(packageName);
            return false;
        }
        
        spdlog::info("[PackageManager] Loaded package: {} v{}", name, version);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("[PackageManager] Error loading package.json for {}: {}", packageName, e.what());
        return false;
    }
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
        // ^1.2.3 means >=1.2.3 and <2.0.0
        // ^0.2.3 means >=0.2.3 and <0.3.0 (special case for 0.x.x)
        // ^0.0.3 means >=0.0.3 and <0.0.4 (special case for 0.0.x)
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
    
    for (const auto& [name, package] : loadedPackages) {
        const auto& components = package->getComponents();
        allComponents.insert(allComponents.end(), components.begin(), components.end());
    }
    
    return allComponents;
}

std::vector<SystemInfo> PackageManager::getAllSystems() const {
    std::vector<SystemInfo> allSystems;
    
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
    auto splitVersion = [](const std::string& version) -> std::vector<int> {
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
    };
    
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

// Helper function for version splitting (used in multiple places)
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

} // namespace GameEngine