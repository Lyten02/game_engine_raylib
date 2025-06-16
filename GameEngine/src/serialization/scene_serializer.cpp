#include "scene_serializer.h"
#include "../scene/scene.h"
#include "dynamic_component_registry.h"
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
    
    // Use dynamic component registry for serialization
    entityJson["components"] = DynamicComponentRegistry::getInstance().serializeComponents(entity, registry);
    
    return entityJson;
}

entt::entity SceneSerializer::jsonToEntity(const nlohmann::json& entityJson, entt::registry& registry) {
    entt::entity entity = registry.create();
    
    if (!entityJson.contains("components")) {
        return entity;
    }
    
    const auto& components = entityJson["components"];
    
    // Use dynamic component registry for deserialization
    DynamicComponentRegistry::getInstance().deserializeComponents(entity, registry, components);
    
    return entity;
}

// TEMP: Keep old code commented for reference
// Old component deserialization code removed - now handled by DynamicComponentRegistry

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