#include "project_validator.h"
#include "../utils/file_utils.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace GameEngine {


ProjectValidator::ValidationResult ProjectValidator::validateProject(const std::string& projectPath) {
    ValidationResult result;
    result.valid = true;
    
    // Check project structure
    if (!validateProjectStructure(projectPath)) {
        result.errors.push_back("Invalid project structure");
        result.valid = false;
    }
    
    // Check project.json
    std::string projectFile = projectPath + "/project.json";
    if (!validateProjectFile(projectFile)) {
        result.errors.push_back("Invalid project.json file");
        result.valid = false;
    }
    
    // Check scene files
    std::string scenesPath = projectPath + "/scenes";
    if (std::filesystem::exists(scenesPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(scenesPath)) {
            if (entry.path().extension() == ".json") {
                if (!validateSceneFile(entry.path().string())) {
                    result.errors.push_back("Invalid scene file: " + entry.path().filename().string());
                    result.valid = false;
                }
            }
        }
    }
    
    // Check resource references
    if (!validateResourceReferences(projectPath)) {
        result.warnings.push_back("Some resource references could not be validated");
    }
    
    return result;
}

bool ProjectValidator::validateProjectStructure(const std::string& projectPath) {
    if (!std::filesystem::exists(projectPath)) {
        spdlog::error("Project path does not exist: {}", projectPath);
        return false;
    }
    
    // Check required files
    if (!std::filesystem::exists(projectPath + "/project.json")) {
        spdlog::error("project.json not found in: {}", projectPath);
        return false;
    }
    
    // Check required directories (create if missing)
    std::vector<std::string> requiredDirs = {"scenes", "assets", "scripts"};
    for (const auto& dir : requiredDirs) {
        std::string dirPath = projectPath + "/" + dir;
        if (!std::filesystem::exists(dirPath)) {
            try {
                std::filesystem::create_directories(dirPath);
                spdlog::info("Created missing directory: {}", dirPath);
            }
            catch (const std::exception& e) {
                spdlog::error("Failed to create directory {}: {}", dirPath, e.what());
                return false;
            }
        }
    }
    
    return true;
}

bool ProjectValidator::validateProjectFile(const std::string& projectFile) {
    if (!std::filesystem::exists(projectFile)) {
        return false;
    }
    
    try {
        std::string content = FileUtils::readFile(projectFile);
        nlohmann::json projectData = nlohmann::json::parse(content);
        
        // Check required fields
        if (!projectData.contains("name") || !projectData["name"].is_string()) {
            spdlog::error("Project file missing 'name' field");
            return false;
        }
        
        if (!projectData.contains("version") || !projectData["version"].is_string()) {
            spdlog::error("Project file missing 'version' field");
            return false;
        }
        
        if (projectData.contains("scenes") && !projectData["scenes"].is_array()) {
            spdlog::error("Project file 'scenes' field must be an array");
            return false;
        }
        
        if (projectData.contains("settings")) {
            if (!projectData["settings"].is_object()) {
                spdlog::error("Project file 'settings' field must be an object");
                return false;
            }
        }
        
        return true;
    }
    catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse project file: {}", e.what());
        return false;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to validate project file: {}", e.what());
        return false;
    }
}

bool ProjectValidator::validateSceneFile(const std::string& sceneFile) {
    if (!std::filesystem::exists(sceneFile)) {
        return false;
    }
    
    try {
        std::string content = FileUtils::readFile(sceneFile);
        nlohmann::json sceneData = nlohmann::json::parse(content);
        
        // Check required fields
        if (!sceneData.contains("name") || !sceneData["name"].is_string()) {
            spdlog::error("Scene file missing 'name' field: {}", sceneFile);
            return false;
        }
        
        if (!sceneData.contains("entities") || !sceneData["entities"].is_array()) {
            spdlog::error("Scene file missing 'entities' array: {}", sceneFile);
            return false;
        }
        
        // Validate entities
        for (const auto& entity : sceneData["entities"]) {
            if (!entity.is_object()) {
                spdlog::error("Invalid entity in scene file: {}", sceneFile);
                return false;
            }
            
            if (!entity.contains("name") || !entity["name"].is_string()) {
                spdlog::error("Entity missing 'name' field in scene file: {}", sceneFile);
                return false;
            }
            
            if (entity.contains("components") && !entity["components"].is_object()) {
                spdlog::error("Entity 'components' must be an object in scene file: {}", sceneFile);
                return false;
            }
        }
        
        return true;
    }
    catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse scene file {}: {}", sceneFile, e.what());
        return false;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to validate scene file {}: {}", sceneFile, e.what());
        return false;
    }
}

bool ProjectValidator::validateResourceReferences(const std::string& projectPath) {
    bool allValid = true;
    
    // Check scene files for resource references
    std::string scenesPath = projectPath + "/scenes";
    if (std::filesystem::exists(scenesPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(scenesPath)) {
            if (entry.path().extension() == ".json") {
                try {
                    std::string content = FileUtils::readFile(entry.path().string());
                    nlohmann::json sceneData = nlohmann::json::parse(content);
                    
                    // Check entities for resource references
                    if (sceneData.contains("entities") && sceneData["entities"].is_array()) {
                        for (const auto& entity : sceneData["entities"]) {
                            if (entity.contains("components") && entity["components"].is_object()) {
                                // Check Sprite component for texture references
                                if (entity["components"].contains("Sprite")) {
                                    const auto& sprite = entity["components"]["Sprite"];
                                    if (sprite.contains("texture") && sprite["texture"].is_string()) {
                                        std::string texturePath = projectPath + "/assets/" + sprite["texture"].get<std::string>();
                                        if (!std::filesystem::exists(texturePath)) {
                                            spdlog::warn("Missing texture resource: {} in scene {}", 
                                                       sprite["texture"].get<std::string>(), 
                                                       entry.path().filename().string());
                                            allValid = false;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                catch (const std::exception& e) {
                    spdlog::warn("Failed to validate resources in scene {}: {}", 
                               entry.path().filename().string(), e.what());
                }
            }
        }
    }
    
    return allValid;
}

} // namespace GameEngine