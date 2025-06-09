#include "project_manager.h"
#include "../utils/file_utils.h"
#include "../utils/string_utils.h"
#include <filesystem>
#include <fstream>
#include <regex>
#include <spdlog/spdlog.h>

namespace GameEngine {


bool ProjectManager::createProject(const std::string& name, const std::string& template_name) {
    // Validate project name
    std::regex validNameRegex("^[a-zA-Z0-9_-]+$");
    if (!std::regex_match(name, validNameRegex)) {
        spdlog::error("Invalid project name: {}. Only letters, numbers, underscores and hyphens are allowed.", name);
        return false;
    }
    
    if (projectExists(name)) {
        spdlog::error("Project already exists: {}", name);
        return false;
    }
    
    try {
        // Create project directory structure
        std::string projectPath = projectsRootPath + name;
        std::filesystem::create_directories(projectPath);
        std::filesystem::create_directories(projectPath + "/scenes");
        std::filesystem::create_directories(projectPath + "/assets");
        std::filesystem::create_directories(projectPath + "/scripts");
        
        // Copy template files
        std::string templatePath = "../templates/" + template_name;
        
        // Load and process project template
        std::string templateFile = templatePath + "/project_template.json";
        if (std::filesystem::exists(templateFile)) {
            std::string content = FileUtils::readFile(templateFile);
            
            // Replace template variables
            content = StringUtils::replace(content, "{{PROJECT_NAME}}", name);
            
            // Parse and update JSON
            nlohmann::json projectData = nlohmann::json::parse(content);
            projectData["name"] = name;
            
            // Save project file
            std::ofstream file(projectPath + "/project.json");
            file << projectData.dump(4);
            file.close();
            
            // Copy scene templates
            std::string scenesTemplatePath = templatePath + "/scenes";
            if (std::filesystem::exists(scenesTemplatePath)) {
                for (const auto& entry : std::filesystem::directory_iterator(scenesTemplatePath)) {
                    if (entry.path().extension() == ".json") {
                        std::string sceneContent = FileUtils::readFile(entry.path().string());
                        std::string sceneName = entry.path().stem().string();
                        
                        std::ofstream sceneFile(projectPath + "/scenes/" + sceneName + ".json");
                        sceneFile << sceneContent;
                        sceneFile.close();
                    }
                }
            }
        } else {
            // Create default project.json if template not found
            nlohmann::json projectData = {
                {"name", name},
                {"version", "1.0.0"},
                {"engine_version", "1.0.0"},
                {"description", "A new game project"},
                {"scenes", nlohmann::json::array()},
                {"settings", {
                    {"window", {
                        {"width", 800},
                        {"height", 600},
                        {"title", name}
                    }},
                    {"physics", {
                        {"gravity", {0, -9.8}}
                    }}
                }}
            };
            
            std::ofstream file(projectPath + "/project.json");
            file << projectData.dump(4);
            file.close();
        }
        
        spdlog::info("Project created: {} at {}", name, projectPath);
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to create project: {}", e.what());
        return false;
    }
}

bool ProjectManager::openProject(const std::string& name) {
    std::string projectPath = projectsRootPath + name;
    
    if (!std::filesystem::exists(projectPath)) {
        spdlog::error("Project not found: {}", name);
        return false;
    }
    
    // Close current project if any
    closeProject();
    
    // Create and load new project
    currentProject = std::make_unique<Project>();
    if (!currentProject->load(projectPath)) {
        currentProject.reset();
        return false;
    }
    
    // Add to recent projects
    auto it = std::find(recentProjects.begin(), recentProjects.end(), name);
    if (it != recentProjects.end()) {
        recentProjects.erase(it);
    }
    recentProjects.insert(recentProjects.begin(), name);
    
    // Keep only last 10 recent projects
    if (recentProjects.size() > 10) {
        recentProjects.resize(10);
    }
    
    spdlog::info("Project opened: {}", name);
    return true;
}

bool ProjectManager::closeProject() {
    if (currentProject) {
        currentProject->save();
        std::string projectName = currentProject->getName();
        currentProject.reset();
        spdlog::info("Project closed: {}", projectName);
        return true;
    }
    return false;
}

std::vector<std::string> ProjectManager::listProjects() {
    std::vector<std::string> projects;
    
    if (!std::filesystem::exists(projectsRootPath)) {
        std::filesystem::create_directories(projectsRootPath);
        return projects;
    }
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(projectsRootPath)) {
            if (entry.is_directory()) {
                std::string projectFile = entry.path().string() + "/project.json";
                if (std::filesystem::exists(projectFile)) {
                    projects.push_back(entry.path().filename().string());
                }
            }
        }
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to list projects: {}", e.what());
    }
    
    return projects;
}

Project* ProjectManager::getCurrentProject() {
    return currentProject.get();
}

bool ProjectManager::projectExists(const std::string& name) {
    std::string projectPath = projectsRootPath + name + "/project.json";
    return std::filesystem::exists(projectPath);
}

} // namespace GameEngine