#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include "package.h"

namespace GameEngine {

// Forward declarations
class PackageLoader;
class PluginManager;

// Dependency resolution result
struct DependencyResolution {
    bool satisfied = true;
    std::vector<std::string> missing;
    std::vector<std::string> incompatible;
    std::vector<std::string> loadOrder;
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
    std::vector<std::string> getLoadedPackages() const;
    
    // Dependency management
    DependencyResolution checkDependencies(const std::string& packageName) const;
    std::vector<std::string> getDependencyOrder(const std::string& packageName) const;
    bool hasCircularDependency(const std::string& packageName) const;
    bool isVersionCompatible(const std::string& requirement, const std::string& version) const;
    
    // Error handling
    std::string getLastError() const { return lastError; }
    
private:
    bool loadPackageMetadata(const std::string& name, const std::filesystem::path& packagePath);
    bool resolveAndLoadDependencies(const std::string& packageName);
    bool checkCircularDependencyRecursive(const std::string& packageName, 
                                         std::unordered_set<std::string>& visited,
                                         std::unordered_set<std::string>& recursionStack) const;
    void buildDependencyOrder(const std::string& packageName,
                            std::unordered_set<std::string>& visited,
                            std::vector<std::string>& order) const;
    
    std::filesystem::path packagesPath;
    std::unordered_map<std::string, std::unique_ptr<Package>> packages;
    std::unordered_map<std::string, std::filesystem::path> availablePackages;
    
    PackageLoader* packageLoader = nullptr;
    PluginManager* pluginManager = nullptr;
    std::string lastError;
};

} // namespace GameEngine