#include "scene_serializer.h"
#include "../scene/scene.h"
#include "../components/transform.h"
#include "../components/sprite.h"
#include "../utils/file_utils.h"
#include "component_registry.h"
#include <fstream>
#include <spdlog/spdlog.h>

namespace GameEngine {

bool SceneSerializer::saveScene(Scene* scene, const std::string& filePath) {
    try {
        nlohmann::json sceneJson = sceneToJson(scene);
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            spdlog::error("Failed to open file for writing: {}", filePath);
            return false;
        }
        
        file << sceneJson.dump(4);
        file.close();
        
        spdlog::info("Scene saved successfully: {}", filePath);
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to save scene: {}", e.what());
        return false;
    }
}

bool SceneSerializer::loadScene(Scene* scene, const std::string& filePath) {
    try {
        if (!fileExists(filePath)) {
            spdlog::error("Scene file not found: {}", filePath);
            return false;
        }
        
        std::string content = FileUtils::readFile(filePath);
        nlohmann::json sceneJson = nlohmann::json::parse(content);
        
        jsonToScene(sceneJson, scene);
        
        spdlog::info("Scene loaded successfully: {}", filePath);
        return true;
    }
    catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse scene JSON: {}", e.what());
        return false;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to load scene: {}", e.what());
        return false;
    }
}

nlohmann::json SceneSerializer::entityToJson(entt::entity entity, entt::registry& registry) {
    nlohmann::json entityJson;
    
    // Get entity ID
    entityJson["id"] = static_cast<uint32_t>(entity);
    
    // Serialize components
    nlohmann::json componentsJson;
    
    // Transform component
    if (registry.all_of<TransformComponent>(entity)) {
        const auto& transform = registry.get<TransformComponent>(entity);
        componentsJson["Transform"] = {
            {"position", {transform.position.x, transform.position.y, transform.position.z}},
            {"rotation", {transform.rotation.x, transform.rotation.y, transform.rotation.z}},
            {"scale", {transform.scale.x, transform.scale.y, transform.scale.z}}
        };
    }
    
    // Sprite component
    if (registry.all_of<Sprite>(entity)) {
        const auto& sprite = registry.get<Sprite>(entity);
        componentsJson["Sprite"] = {
            {"texture", sprite.texturePath},
            {"source", {sprite.sourceRect.x, sprite.sourceRect.y, sprite.sourceRect.width, sprite.sourceRect.height}},
            {"tint", {sprite.tint.r, sprite.tint.g, sprite.tint.b, sprite.tint.a}}
        };
    }
    
    // Add more components here as needed
    
    entityJson["components"] = componentsJson;
    
    return entityJson;
}

entt::entity SceneSerializer::jsonToEntity(const nlohmann::json& entityJson, entt::registry& registry) {
    entt::entity entity = registry.create();
    
    if (!entityJson.contains("components")) {
        return entity;
    }
    
    const auto& components = entityJson["components"];
    
    // Transform component
    if (components.contains("Transform")) {
        const auto& transformJson = components["Transform"];
        auto& transform = registry.emplace<TransformComponent>(entity);
        
        if (transformJson.contains("position")) {
            const auto& pos = transformJson["position"];
            transform.position = {pos[0], pos[1], pos[2]};
        }
        if (transformJson.contains("rotation")) {
            const auto& rot = transformJson["rotation"];
            transform.rotation = {rot[0], rot[1], rot[2]};
        }
        if (transformJson.contains("scale")) {
            const auto& scale = transformJson["scale"];
            transform.scale = {scale[0], scale[1], scale[2]};
        }
    }
    
    // Sprite component
    if (components.contains("Sprite")) {
        const auto& spriteJson = components["Sprite"];
        auto& sprite = registry.emplace<Sprite>(entity);
        
        if (spriteJson.contains("texture")) {
            sprite.texturePath = spriteJson["texture"];
            // Note: Actual texture loading would happen elsewhere
        }
        if (spriteJson.contains("source")) {
            const auto& src = spriteJson["source"];
            sprite.sourceRect = {src[0], src[1], src[2], src[3]};
        }
        if (spriteJson.contains("tint")) {
            const auto& tint = spriteJson["tint"];
            sprite.tint = {
                static_cast<unsigned char>(tint[0]),
                static_cast<unsigned char>(tint[1]),
                static_cast<unsigned char>(tint[2]),
                static_cast<unsigned char>(tint[3])
            };
        }
    }
    
    return entity;
}

nlohmann::json SceneSerializer::sceneToJson(Scene* scene) {
    nlohmann::json sceneJson;
    
    sceneJson["scene_name"] = "Scene"; // TODO: Add scene name to Scene class
    sceneJson["version"] = "1.0.0";
    
    nlohmann::json entitiesJson = nlohmann::json::array();
    
    // Serialize all entities
    auto view = scene->registry.view<entt::entity>();
    for (auto entity : view) {
        nlohmann::json entityJson = entityToJson(entity, scene->registry);
        entitiesJson.push_back(entityJson);
    }
    
    sceneJson["entities"] = entitiesJson;
    
    return sceneJson;
}

void SceneSerializer::jsonToScene(const nlohmann::json& sceneJson, Scene* scene) {
    // Clear existing entities
    scene->registry.clear();
    
    // Check version compatibility
    if (sceneJson.contains("version")) {
        std::string version = sceneJson["version"];
        spdlog::info("Loading scene version: {}", version);
    }
    
    // Load entities
    if (sceneJson.contains("entities") && sceneJson["entities"].is_array()) {
        for (const auto& entityJson : sceneJson["entities"]) {
            jsonToEntity(entityJson, scene->registry);
        }
    }
}

template<typename T>
void SceneSerializer::registerComponent(const std::string& componentName) {
    ComponentRegistry::getInstance().registerComponent<T>(componentName);
}

// Explicit instantiations
template void SceneSerializer::registerComponent<TransformComponent>(const std::string&);
template void SceneSerializer::registerComponent<Sprite>(const std::string&);

} // namespace GameEngine