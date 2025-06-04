#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <entt/entt.hpp>

namespace GameEngine {

class ComponentRegistry {
public:
    using SerializeFunc = std::function<nlohmann::json(entt::entity, entt::registry&)>;
    using DeserializeFunc = std::function<void(entt::entity, entt::registry&, const nlohmann::json&)>;
    
    struct ComponentInfo {
        std::string name;
        SerializeFunc serialize;
        DeserializeFunc deserialize;
    };
    
    static ComponentRegistry& getInstance() {
        static ComponentRegistry instance;
        return instance;
    }
    
    template<typename T>
    void registerComponent(const std::string& name);
    
    bool hasComponent(const std::string& name) const;
    nlohmann::json serializeComponent(const std::string& name, entt::entity entity, entt::registry& registry);
    void deserializeComponent(const std::string& name, entt::entity entity, entt::registry& registry, const nlohmann::json& data);
    
    std::vector<std::string> getRegisteredComponents() const;
    
private:
    ComponentRegistry() = default;
    std::unordered_map<std::string, ComponentInfo> components;
};

// Macro for easy component registration
#define REGISTER_COMPONENT(ComponentType) \
    ComponentRegistry::getInstance().registerComponent<ComponentType>(#ComponentType)

} // namespace GameEngine