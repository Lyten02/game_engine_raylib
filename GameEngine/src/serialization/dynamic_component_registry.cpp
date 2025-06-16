#include "dynamic_component_registry.h"
#include <spdlog/spdlog.h>

namespace GameEngine {

void DynamicComponentRegistry::registerComponent(const std::string& name,
                                               SerializeFunc serialize,
                                               DeserializeFunc deserialize,
                                               CreateFunc create) {
    ComponentInfo info;
    info.name = name;
    info.serialize = serialize;
    info.deserialize = deserialize;
    info.create = create;
    
    components[name] = info;
    spdlog::info("DynamicComponentRegistry: Registered component '{}'", name);
}

bool DynamicComponentRegistry::isRegistered(const std::string& name) const {
    return components.find(name) != components.end();
}

const DynamicComponentRegistry::ComponentInfo* 
DynamicComponentRegistry::getComponentInfo(const std::string& name) const {
    auto it = components.find(name);
    return it != components.end() ? &it->second : nullptr;
}

std::vector<std::string> DynamicComponentRegistry::getRegisteredComponents() const {
    std::vector<std::string> result;
    result.reserve(components.size());
    for (const auto& [name, info] : components) {
        result.push_back(name);
    }
    return result;
}

nlohmann::json DynamicComponentRegistry::serializeComponents(entt::entity entity, 
                                                            entt::registry& registry) const {
    nlohmann::json result;
    
    for (const auto& [name, info] : components) {
        try {
            auto componentData = info.serialize(entity, registry);
            if (!componentData.is_null() && !componentData.empty()) {
                result[name] = componentData;
            }
        } catch (const std::exception& e) {
            spdlog::debug("Component '{}' not present on entity or serialization failed: {}", 
                         name, e.what());
        }
    }
    
    return result;
}

void DynamicComponentRegistry::deserializeComponents(entt::entity entity,
                                                    entt::registry& registry,
                                                    const nlohmann::json& data) const {
    for (const auto& [componentName, componentData] : data.items()) {
        auto it = components.find(componentName);
        if (it != components.end()) {
            try {
                it->second.deserialize(entity, registry, componentData);
            } catch (const std::exception& e) {
                spdlog::error("Failed to deserialize component '{}': {}", 
                             componentName, e.what());
            }
        } else {
            spdlog::warn("Unknown component type '{}' in serialized data", componentName);
        }
    }
}

} // namespace GameEngine