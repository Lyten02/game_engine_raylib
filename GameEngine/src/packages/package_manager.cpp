#include "package_manager.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

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
        
        // Create Package object
        auto package = std::make_shared<Package>(name, version);
        package->setDescription(description);
        
        // Load dependencies if present
        if (packageJson.contains("dependencies")) {
            for (auto& [depName, depVersion] : packageJson["dependencies"].items()) {
                package->addDependency(depName, depVersion);
            }
        }
        
        // Store the loaded package
        loadedPackages[packageName] = package;
        
        spdlog::info("[PackageManager] Loaded package: {} v{}", name, version);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("[PackageManager] Error loading package.json for {}: {}", packageName, e.what());
        return false;
    }
}