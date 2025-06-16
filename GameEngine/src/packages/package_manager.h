#pragma once

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>
#include <optional>
#include "package.h"

struct PackageInfo {
    std::string name;
    std::string version;
    std::string description;
};

class PackageManager {
public:
    explicit PackageManager(const std::filesystem::path& packagesDir);
    
    // Package discovery
    void scanPackages();
    std::vector<std::string> getAvailablePackages() const;
    
    // Package loading
    bool loadPackage(const std::string& packageName);
    bool unloadPackage(const std::string& packageName);
    
    // Package information
    std::shared_ptr<Package> getPackage(const std::string& packageName) const;
    std::vector<std::string> getLoadedPackages() const;
    std::optional<PackageInfo> getPackageInfo(const std::string& packageName) const;
    
    // Version validation
    bool isVersionCompatible(const std::string& required, const std::string& actual) const;
    bool checkEngineCompatibility(const Package& package) const;
    
    // Package details
    std::vector<ComponentInfo> getAllComponents() const;
    std::vector<SystemInfo> getAllSystems() const;
    
    // Getters
    const std::filesystem::path& getPackagesDirectory() const { return packagesDirectory; }
    void setEngineVersion(const std::string& version) { currentEngineVersion = version; }
    
private:
    std::filesystem::path packagesDirectory;
    std::unordered_map<std::string, std::shared_ptr<Package>> loadedPackages;
    std::vector<std::string> availablePackages;
    std::string currentEngineVersion = "0.1.0";
    
    bool loadPackageMetadata(const std::string& packageName, const std::filesystem::path& packagePath);
    bool parseVersionRequirement(const std::string& requirement, std::string& op, std::string& version) const;
    int compareVersions(const std::string& v1, const std::string& v2) const;
    std::vector<int> splitVersion(const std::string& version) const;
};