#pragma once

#include <string>
#include <vector>

struct PackageDependency {
    std::string name;
    std::string version;
};

struct ComponentInfo {
    std::string name;
    std::string file;
};

struct SystemInfo {
    std::string name;
    std::string file;
    int priority = 0;
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
    const std::vector<PackageDependency>& getDependencies() const { return dependencies; }
    const std::vector<ComponentInfo>& getComponents() const { return components; }
    const std::vector<SystemInfo>& getSystems() const { return systems; }
    
    // Setters
    void setDescription(const std::string& desc) { description = desc; }
    void setAuthor(const std::string& auth) { author = auth; }
    void setLicense(const std::string& lic) { license = lic; }
    void setEngineVersion(const std::string& ver) { engineVersion = ver; }
    void addDependency(const std::string& name, const std::string& version);
    void addComponent(const ComponentInfo& component) { components.push_back(component); }
    void addSystem(const SystemInfo& system) { systems.push_back(system); }
    
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
};