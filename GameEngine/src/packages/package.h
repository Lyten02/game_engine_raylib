#pragma once

#include <string>
#include <vector>
#include <memory>

namespace GameEngine {

// Package dependency information
struct PackageDependency {
    std::string name;
    std::string version; // Version requirement (e.g., ">=1.0.0")
};

// Component information
struct ComponentInfo {
    std::string name;
    std::string file;
};

// System information  
struct SystemInfo {
    std::string name;
    std::string file;
    int priority = 0;
};

// Plugin information for packages
struct PackagePluginInfo {
    std::string library;     // Library filename
    std::string main;        // Main class name (optional)
    bool autoload = true;    // Auto-load on package load
};

class Package {
public:
    Package(const std::string& name, const std::string& version);
    
    // Getters
    const std::string& getName() const { return name; }
    const std::string& getVersion() const { return version; }
    const std::string& getDescription() const { return description; }
    const std::string& getAuthor() const { return author; }
    const std::string& getLicense() const { return license; }
    const std::string& getEngineVersion() const { return engineVersion; }
    
    // Setters
    void setDescription(const std::string& desc) { description = desc; }
    void setAuthor(const std::string& auth) { author = auth; }
    void setLicense(const std::string& lic) { license = lic; }
    void setEngineVersion(const std::string& ver) { engineVersion = ver; }
    
    // Dependencies
    void addDependency(const std::string& name, const std::string& version);
    const std::vector<PackageDependency>& getDependencies() const { return dependencies; }
    
    // Components
    void addComponent(const ComponentInfo& component);
    const std::vector<ComponentInfo>& getComponents() const { return components; }
    
    // Systems
    void addSystem(const SystemInfo& system);
    const std::vector<SystemInfo>& getSystems() const { return systems; }
    
    // Plugin
    void setPluginInfo(const PackagePluginInfo& info) { pluginInfo = info; hasPlugin = true; }
    bool hasPluginInfo() const { return hasPlugin; }
    const PackagePluginInfo& getPluginInfo() const { return pluginInfo; }
    
private:
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string license;
    std::string engineVersion;
    
    std::vector<PackageDependency> dependencies;
    std::vector<ComponentInfo> components;
    std::vector<SystemInfo> systems;
    
    bool hasPlugin = false;
    PackagePluginInfo pluginInfo;
};

} // namespace GameEngine