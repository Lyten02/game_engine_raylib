#pragma once

#include <string>
#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

class Scene;

namespace GameEngine {

class SceneSerializer {
public:
    static bool saveScene(Scene* scene, const std::string& filePath);
    static bool loadScene(Scene* scene, const std::string& filePath);
    
    static nlohmann::json entityToJson(entt::entity entity, entt::registry& registry);
    static entt::entity jsonToEntity(const nlohmann::json& entityJson, entt::registry& registry);
    
    template<typename T>
    static void registerComponent(const std::string& componentName);
    
private:
    static nlohmann::json sceneToJson(Scene* scene);
    static void jsonToScene(const nlohmann::json& sceneJson, Scene* scene);
};

} // namespace GameEngine