#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include <entt/entt.hpp>
#include "../systems/system.h"

namespace GameEngine {

// Component factory function type
using ComponentFactory = std::function<void(entt::registry&, entt::entity)>;

// System factory function type
using SystemFactory = std::function<std::unique_ptr<ISystem>()>;

class PackageLoader {
public:
    PackageLoader() = default;
    ~PackageLoader() = default;
    
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
};

} // namespace GameEngine