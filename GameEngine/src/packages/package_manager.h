#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <optional>
#include "package.h"
#include "package_loader.h"

namespace GameEngine {
// Forward declarations
class PluginManager;

// Dependency resolution result
struct DependencyResolution {
    bool satisfied = true;
    std::vector<std::string> missing;
    std::vector<std::string> incompatible;
    std::vector<std::string> loadOrder;
};

struct PackageInfo {
    std::string name;
    std::string version;
    std::string description;
};

class PackageManager {
public:
    explicit PackageManager(const std::filesystem::path& packagesPath);
    ~PackageManager();

    // Set loader and plugin manager
    void setPackageLoader(PackageLoader* loader) { packageLoader = loader; }
    void setPluginManager(PluginManager* manager) { pluginManager = manager; }

    // Package discovery
    void scanPackages();
    std::vector<std::string> getAvailablePackages() const;

    // Package loading
    bool loadPackage(const std::string& name);
    bool loadPackageWithDependencies(const std::string& name);
    bool unloadPackage(const std::string& name);

    // Package queries
    Package* getPackage(const std::string& name);
    const Package* getPackage(const std::string& name) const;
    std::shared_ptr<Package> getPackageShared(const std::string& packageName) const;
    std::vector<std::string> getLoadedPackages() const;
    std::optional<PackageInfo> getPackageInfo(const std::string& packageName) const;

    // Dependency resolution
    DependencyResolution checkDependencies(const std::string& packageName) const;
    bool isVersionCompatible(const std::string& required, const std::string& actual) const;
    bool checkEngineCompatibility(const Package& package) const;
    bool hasCircularDependency(const std::string& packageName) const;
    std::vector<std::string> getDependencyOrder(const std::string& packageName) const;

    // Package details
    std::vector<ComponentInfo> getAllComponents() const;
    std::vector<SystemInfo> getAllSystems() const;

    // Getters
    const std::filesystem::path& getPackagesDirectory() const { return packagesPath; }
    void setEngineVersion(const std::string& version) { currentEngineVersion = version; }
    PackageLoader& getPackageLoader() { return internalPackageLoader; }
    const PackageLoader& getPackageLoader() const { return internalPackageLoader; }

    // Error handling
    std::string getLastError() const { return lastError; }

private:
    std::filesystem::path packagesPath;
    std::unordered_map<std::string, std::unique_ptr<Package >> packages;
    std::unordered_map<std::string, std::shared_ptr<Package >> loadedPackages;
    std::unordered_map<std::string, std::filesystem::path> availablePackages;
    std::string currentEngineVersion = "0.1.0";

    PackageLoader* packageLoader = nullptr;
    PackageLoader internalPackageLoader; // Internal loader for static packages
    PluginManager* pluginManager = nullptr;
    std::string lastError;

    bool loadPackageMetadata(const std::string& name, const std::filesystem::path& packagePath);
    bool parseVersionRequirement(const std::string& requirement, std::string& op, std::string& version) const;
    int compareVersions(const std::string& v1, const std::string& v2) const;
    std::vector<int> splitVersion(const std::string& version) const;
};

} // namespace GameEngine
