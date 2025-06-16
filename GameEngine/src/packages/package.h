#pragma once

#include <string>
#include <vector>

struct PackageDependency {
    std::string name;
    std::string version;
};

class Package {
public:
    Package(const std::string& name, const std::string& version);
    
    // Getters
    const std::string& getName() const { return name; }
    const std::string& getVersion() const { return version; }
    const std::string& getDescription() const { return description; }
    const std::vector<PackageDependency>& getDependencies() const { return dependencies; }
    
    // Setters
    void setDescription(const std::string& desc) { description = desc; }
    void addDependency(const std::string& name, const std::string& version);
    
private:
    std::string name;
    std::string version;
    std::string description;
    std::vector<PackageDependency> dependencies;
};