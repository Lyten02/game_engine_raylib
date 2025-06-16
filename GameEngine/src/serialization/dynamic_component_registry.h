#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

// Remove namespace to avoid conflicts

// Dynamic component registry that doesn't know about specific component types
class DynamicComponentRegistry {
public:
    using SerializeFunc = std::function<nlohmann::json(entt::entity, entt::registry&)>;
    using DeserializeFunc = std::function<void(entt::entity, entt::registry&, const nlohmann::json&)>;
    using CreateFunc = std::function<void(entt::entity, entt::registry&)>;
    
    struct ComponentInfo {
        std::string name;
        SerializeFunc serialize;
        DeserializeFunc deserialize;
        CreateFunc create;
    };
    
    static DynamicComponentRegistry& getInstance() {
        static DynamicComponentRegistry instance;
        return instance;
    }
    
    // Register component with factory functions
    void registerComponent(const std::string& name, 
                          SerializeFunc serialize,
                          DeserializeFunc deserialize,
                          CreateFunc create);
    
    // Check if component is registered
    bool isRegistered(const std::string& name) const;
    
    // Get component info
    const ComponentInfo* getComponentInfo(const std::string& name) const;
    
    // Get all registered components
    std::vector<std::string> getRegisteredComponents() const;
    
    // Serialize entity components
    nlohmann::json serializeComponents(entt::entity entity, entt::registry& registry) const;
    
    // Deserialize entity components
    void deserializeComponents(entt::entity entity, entt::registry& registry, const nlohmann::json& data) const;
    
private:
    DynamicComponentRegistry() = default;
    std::unordered_map<std::string, ComponentInfo> components;
};