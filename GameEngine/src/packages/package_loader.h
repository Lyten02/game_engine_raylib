#pragma once

#include <filesystem>
#include <memory>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <entt/entt.hpp>

namespace GameEngine {

// Forward declarations
class Package;
struct ComponentInfo;
struct SystemInfo;

// Component factory function type
using ComponentFactory = std::function<void(entt::registry&, entt::entity)>;

// System factory function type  
class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void initialize() = 0;
    virtual void update(entt::registry& registry, float deltaTime) = 0;
    virtual void shutdown() = 0;
};

using SystemFactory = std::function<std::unique_ptr<ISystem>()>;

class PackageLoader {
public:
    PackageLoader();
    ~PackageLoader();
    
    // Load package resources
    bool loadPackageResources(const Package& package, const std::filesystem::path& packagePath);
    
    // Component registration
    void registerComponent(const std::string& name, ComponentFactory factory);
    bool hasComponent(const std::string& name) const;
    ComponentFactory getComponentFactory(const std::string& name) const;
    
    // System registration
    void registerSystem(const std::string& name, SystemFactory factory);
    bool hasSystem(const std::string& name) const;
    SystemFactory getSystemFactory(const std::string& name) const;
    
    // Get all registered items
    std::vector<std::string> getRegisteredComponents() const;
    std::vector<std::string> getRegisteredSystems() const;
    
private:
    std::unordered_map<std::string, ComponentFactory> componentFactories;
    std::unordered_map<std::string, SystemFactory> systemFactories;
    
    // Built-in registrations
    void registerBuiltinComponents();
    void registerBuiltinSystems();
    
    // Dynamic loading (for future use with shared libraries)
    bool loadComponentFromFile(const ComponentInfo& component, const std::filesystem::path& basePath);
    bool loadSystemFromFile(const SystemInfo& system, const std::filesystem::path& basePath);
};

} // namespace GameEngine