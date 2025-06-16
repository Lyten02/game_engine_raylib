#include "project.h"
#include "../utils/file_utils.h"
#include "../utils/log_limiter.h"
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace GameEngine {

bool Project::load(const std::string& projectPath) {
    path = projectPath;
    std::string projectFile = projectPath + "/project.json";
    
    if (!std::filesystem::exists(projectFile)) {
        spdlog::error("Project file not found: {}", projectFile);
        return false;
    }
    
    try {
        std::string jsonContent = FileUtils::readFile(projectFile);
        metadata = nlohmann::json::parse(jsonContent);
        
        // Load project metadata
        name = metadata.value("name", "Unnamed Project");
        version = metadata.value("version", "1.0.0");
        
        // Load scenes list
        scenes.clear();
        if (metadata.contains("scenes") && metadata["scenes"].is_array()) {
            for (const auto& scene : metadata["scenes"]) {
                if (scene.is_string()) {
                    scenes.push_back(scene.get<std::string>());
                }
            }
        }
        
        // Load start scene
        startScene = metadata.value("start_scene", "");
        if (!startScene.empty()) {
            spdlog::info("Project start scene: {}", startScene);
        }
        
        // Validate project structure
        std::filesystem::create_directories(path + "/scenes");
        std::filesystem::create_directories(path + "/assets");
        std::filesystem::create_directories(path + "/scripts");
        
        // Use log limiter to prevent spam when loading many projects
        LogLimiter::info("project_loaded", "Project loaded: {} (version {})", name, version);
        return true;
    }
    catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse project file: {}", e.what());
        return false;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to load project: {}", e.what());
        return false;
    }
}

bool Project::save() {
    if (path.empty()) {
        spdlog::error("Cannot save project: path is empty");
        return false;
    }
    
    try {
        // Update metadata
        metadata["name"] = name;
        metadata["version"] = version;
        metadata["scenes"] = scenes;
        if (!startScene.empty()) {
            metadata["start_scene"] = startScene;
        }
        
        // Save to file
        std::string projectFile = path + "/project.json";
        std::ofstream file(projectFile);
        if (!file.is_open()) {
            spdlog::error("Failed to open project file for writing: {}", projectFile);
            return false;
        }
        
        file << metadata.dump(4);
        file.close();
        
        spdlog::info("Project saved: {}", projectFile);
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to save project: {}", e.what());
        return false;
    }
}

bool Project::createScene(const std::string& sceneName) {
    // Check if scene already exists
    for (const auto& scene : scenes) {
        if (scene == sceneName) {
            spdlog::warn("Scene already exists: {}", sceneName);
            return false;
        }
    }
    
    try {
        // Create scene file
        std::string scenePath = path + "/scenes/" + sceneName + ".json";
        
        nlohmann::json sceneData = {
            {"name", sceneName},
            {"entities", nlohmann::json::array()}
        };
        
        std::ofstream file(scenePath);
        if (!file.is_open()) {
            spdlog::error("Failed to create scene file: {}", scenePath);
            return false;
        }
        
        file << sceneData.dump(4);
        file.close();
        
        // Add to scenes list
        scenes.push_back(sceneName);
        
        // Save project
        save();
        
        spdlog::info("Scene created: {}", sceneName);
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to create scene: {}", e.what());
        return false;
    }
}

bool Project::deleteScene(const std::string& sceneName) {
    auto it = std::find(scenes.begin(), scenes.end(), sceneName);
    if (it == scenes.end()) {
        spdlog::warn("Scene not found: {}", sceneName);
        return false;
    }
    
    try {
        // Delete scene file
        std::string scenePath = path + "/scenes/" + sceneName + ".json";
        if (std::filesystem::exists(scenePath)) {
            std::filesystem::remove(scenePath);
        }
        
        // Remove from scenes list
        scenes.erase(it);
        
        // Save project
        save();
        
        spdlog::info("Scene deleted: {}", sceneName);
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to delete scene: {}", e.what());
        return false;
    }
}

std::vector<std::string> Project::getScenes() const {
    return scenes;
}

const std::string& Project::getName() const {
    return name;
}

const std::string& Project::getPath() const {
    return path;
}

std::string Project::getGameLogic() const {
    return metadata.value("game_logic", "");
}

} // namespace GameEngine