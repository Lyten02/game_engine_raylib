#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <typeindex>
#include <entt/entt.hpp>
#include "../systems/system.h"
#include "package.h"

namespace GameEngine {
// Forward declarations
class PluginManager;
class Package;
struct ComponentInfo;
struct SystemInfo;

// Component factory function type
using ComponentFactory = std::function<void(entt::registry&, entt::entity)>;

// System factory function type
using SystemFactory = std::function<std::unique_ptr<ISystem>()>;

class PackageLoader {
public:
    PackageLoader();
    ~PackageLoader();

    // Set plugin manager for loading plugins
    void setPluginManager(PluginManager* manager) { pluginManager = manager; }

    // Load package resources (components, systems, and plugins)
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

    // Error handling
    std::string getLastError() const { return lastError; }

private:
    std::unordered_map<std::string, ComponentFactory> componentFactories;
    std::unordered_map<std::string, SystemFactory> systemFactories;
    PluginManager* pluginManager = nullptr;
    std::string lastError;

    // Built-in registrations
    void registerBuiltinComponents();
    void registerBuiltinSystems();

    // Dynamic loading (for future use with shared libraries)
    bool loadComponentFromFile(const ComponentInfo& component, const std::filesystem::path& basePath);
    bool loadSystemFromFile(const SystemInfo& system, const std::filesystem::path& basePath);
};

} // namespace GameEngine
