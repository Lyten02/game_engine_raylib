#include "component_registry.h"
#include "../components/transform.h"
#include "../components/sprite.h"
#include <spdlog/spdlog.h>

namespace GameEngine {

template<>
void ComponentRegistry::registerComponent<TransformComponent>(const std::string& name) {
    ComponentInfo info;
    info.name = name;
    
    info.serialize = [](entt::entity entity, entt::registry& registry) -> nlohmann::json {
        if (!registry.all_of<TransformComponent>(entity)) {
            return {};
        }
        
        const auto& transform = registry.get<TransformComponent>(entity);
        return {
            {"position", {transform.position.x, transform.position.y, transform.position.z}},
            {"rotation", {transform.rotation.x, transform.rotation.y, transform.rotation.z}},
            {"scale", {transform.scale.x, transform.scale.y, transform.scale.z}}
        };
    };
    
    info.deserialize = [](entt::entity entity, entt::registry& registry, const nlohmann::json& data) {
        auto& transform = registry.emplace_or_replace<TransformComponent>(entity);
        
        if (data.contains("position")) {
            const auto& pos = data["position"];
            transform.position = {pos[0], pos[1], pos[2]};
        }
        if (data.contains("rotation")) {
            const auto& rot = data["rotation"];
            transform.rotation = {rot[0], rot[1], rot[2]};
        }
        if (data.contains("scale")) {
            const auto& scale = data["scale"];
            transform.scale = {scale[0], scale[1], scale[2]};
        }
    };
    
    components[name] = info;
    spdlog::info("Registered component: {}", name);
}

template<>
void ComponentRegistry::registerComponent<Sprite>(const std::string& name) {
    ComponentInfo info;
    info.name = name;
    
    info.serialize = [](entt::entity entity, entt::registry& registry) -> nlohmann::json {
        if (!registry.all_of<Sprite>(entity)) {
            return {};
        }
        
        const auto& sprite = registry.get<Sprite>(entity);
        return {
            {"texture", sprite.texturePath},
            {"source", {sprite.sourceRect.x, sprite.sourceRect.y, sprite.sourceRect.width, sprite.sourceRect.height}},
            {"tint", {sprite.tint.r, sprite.tint.g, sprite.tint.b, sprite.tint.a}}
        };
    };
    
    info.deserialize = [](entt::entity entity, entt::registry& registry, const nlohmann::json& data) {
        auto& sprite = registry.emplace_or_replace<Sprite>(entity);
        
        if (data.contains("texture")) {
            sprite.texturePath = data["texture"];
        }
        if (data.contains("source")) {
            const auto& src = data["source"];
            sprite.sourceRect = {src[0], src[1], src[2], src[3]};
        }
        if (data.contains("tint")) {
            const auto& tint = data["tint"];
            sprite.tint = {
                static_cast<unsigned char>(tint[0]),
                static_cast<unsigned char>(tint[1]),
                static_cast<unsigned char>(tint[2]),
                static_cast<unsigned char>(tint[3])
            };
        }
    };
    
    components[name] = info;
    spdlog::info("Registered component: {}", name);
}

bool ComponentRegistry::hasComponent(const std::string& name) const {
    return components.find(name) != components.end();
}

nlohmann::json ComponentRegistry::serializeComponent(const std::string& name, entt::entity entity, entt::registry& registry) {
    auto it = components.find(name);
    if (it != components.end()) {
        return it->second.serialize(entity, registry);
    }
    return {};
}

void ComponentRegistry::deserializeComponent(const std::string& name, entt::entity entity, entt::registry& registry, const nlohmann::json& data) {
    auto it = components.find(name);
    if (it != components.end()) {
        it->second.deserialize(entity, registry, data);
    } else {
        spdlog::warn("Component type not registered: {}", name);
    }
}

std::vector<std::string> ComponentRegistry::getRegisteredComponents() const {
    std::vector<std::string> names;
    for (const auto& [name, info] : components) {
        names.push_back(name);
    }
    return names;
}

} // namespace GameEngine