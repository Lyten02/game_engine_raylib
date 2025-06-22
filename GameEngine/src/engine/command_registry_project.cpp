#include "command_registry.h"
#include "../console/console.h"
#include "../console/command_processor.h"
#include "../project/project_manager.h"
#include "../project/project.h"
#include "../scene/scene.h"
#include "../engine.h"
#include "../serialization/scene_serializer.h"
#include <filesystem>
#include <spdlog/spdlog.h>

namespace GameEngine {
void CommandRegistry::registerProjectCommands(CommandProcessor* processor, Console* console,
                                           ProjectManager* projectManager, std::function<Scene*()> getScene, Engine* engine) {
    // project.create command
        std::vector<CommandParameter> projectParams = {{"name", "Name of the new project", true}};
        processor->registerCommand("project.create",
        [console, projectManager, getScene, engine](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: project.create <name>", RED);
                return;
            }

            // Check if project already exists
            if (projectManager->projectExists(args[0])) {
                console->addLine("Project already exists: " + args[0], YELLOW);
                console->addLine("Opening existing project...", GRAY);

                // Open the existing project
                if (projectManager->openProject(args[0])) {
                    console->addLine("Project opened: " + args[0], GREEN);

                    // Destroy current scene
                    if (getScene()) {
                        engine->destroyScene();
                    }

                    // Create new scene
                    engine->createScene();

                    // Try to load the main scene
                    std::string mainScenePath = projectManager->getCurrentProject()->getPath() + "/scenes/main_scene.json";
                    if (std::filesystem::exists(mainScenePath)) {
                        SceneSerializer serializer;
                        if (serializer.loadScene(getScene(), mainScenePath)) {
                            console->addLine("Loaded main scene", GRAY);
                        }
                    }
                } else {
                    console->addLine("Failed to open existing project: " + args[0], RED);
                }
                return;
            }

            // Create new project if it doesn't exist
            if (projectManager->createProject(args[0])) {
                console->addLine("Project created: " + args[0], GREEN);
                console->addLine("Use 'project.open " + args[0] + "' to open it", YELLOW);
            } else {
                console->addLine("Failed to create project: " + args[0], RED);
            }
        }, "Create a new project or open if it already exists", "Project",
        "project.create <name>", projectParams);

    // project.open command
    std::vector<CommandParameter> openParams = {
        {"name", "Name of the project to open", true, [projectManager]() { 
            return projectManager->listProjects(); 
        }}
    };
    processor->registerCommand("project.open",
        [console, projectManager, getScene, engine](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: project.open <name>", RED);
                return;
            }

            if (projectManager->openProject(args[0])) {
                console->addLine("Project opened: " + args[0], GREEN);

                // Destroy current scene
                if (getScene()) {
                    engine->destroyScene();
                }

                // Create new scene
                engine->createScene();

                // Try to load the main scene
                std::string mainScenePath = projectManager->getCurrentProject()->getPath() + "/scenes/main_scene.json";
                if (std::filesystem::exists(mainScenePath)) {
                    SceneSerializer serializer;
                    if (serializer.loadScene(getScene(), mainScenePath)) {
                        console->addLine("Loaded main scene", GRAY);
                    }
                }
            } else {
                console->addLine("Failed to open project: " + args[0], RED);
            }
        }, "Open an existing project", "Project",
        "project.open <name>", openParams);

    // project.close command
    processor->registerCommand("project.close",
        [console, projectManager, getScene, engine](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project is currently open", YELLOW);
                return;
            }

            std::string projectName = projectManager->getCurrentProject()->getName();

            // Destroy current scene
            if (getScene()) {
                engine->destroyScene();
            }

            projectManager->closeProject();
            console->addLine("Project closed: " + projectName, GREEN);
        }, "Close the current project", "Project");

    // project.list command
    processor->registerCommand("project.list",
        [console, projectManager](const std::vector<std::string>& args) {
            std::vector<std::string> projects = projectManager->listProjects();

            if (projects.empty()) {
                console->addLine("No projects found", YELLOW);
                console->addLine("Use 'project.create <name>' to create a new project", GRAY);
            } else {
                console->addLine("Available projects:", YELLOW);
                for (const auto& proj : projects) {
                    console->addLine("  - " + proj, WHITE);
                }
            }
        }, "List all projects", "Project");

    // project.info command
    processor->registerCommand("project.info",
        [console, projectManager](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project is currently open", YELLOW);
                return;
            }

            auto project = projectManager->getCurrentProject();
            console->addLine("Project Information:", YELLOW);
            console->addLine("  Name: " + project->getName(), WHITE);
            console->addLine("  Path: " + project->getPath(), WHITE);
            console->addLine("  Build Path: output/" + project->getName(), WHITE);
            console->addLine("  Assets Path: " + project->getPath() + "/assets", WHITE);
        }, "Show current project information", "Project");

    // project.rename command
    std::vector<CommandParameter> renameParams = {{"new_name", "New project name", true}};
    processor->registerCommand("project.rename",
        [console, projectManager](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project is currently open", YELLOW);
                return;
            }

            if (args.empty()) {
                console->addLine("Usage: project.rename <new_name>", RED);
                return;
            }

            std::string oldName = projectManager->getCurrentProject()->getName();
            // Rename not implemented
            console->addLine("Project rename not implemented yet", YELLOW);
            /*if (projectManager->renameProject(args[0])) {
                console->addLine("Project renamed from '" + oldName + "' to '" + args[0] + "'", GREEN);
            } else {
                console->addLine("Failed to rename project", RED);
            }*/
        }, "Rename the current project", "Project",
        "project.rename <new_name>", renameParams);

    // project.delete command
    std::vector<CommandParameter> deleteParams = {
        {"name", "Name of the project to delete", true, [projectManager]() { 
            return projectManager->listProjects(); 
        }}
    };
    processor->registerCommand("project.delete",
        [console, projectManager](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: project.delete <name>", RED);
                console->addLine("WARNING: This will permanently delete the project and all its files!", YELLOW);
                return;
            }

            // Prevent deleting current project
            if (projectManager->getCurrentProject() &&
                projectManager->getCurrentProject()->getName() == args[0]) {
                console->addLine("Cannot delete the currently open project. Close it first.", RED);
                return;
            }

            // Delete not implemented, do it manually
            std::string projectPath = "projects/" + args[0];
            if (std::filesystem::exists(projectPath)) {
                try {
                    std::filesystem::remove_all(projectPath);
                    console->addLine("Project deleted: " + args[0], GREEN);
                } catch (const std::exception& e) {
                    console->addLine("Failed to delete project: " + std::string(e.what()), RED);
                }
            } else {
                console->addLine("Project not found: " + args[0], RED);
            }
            /*if (projectManager->deleteProject(args[0])) {
                console->addLine("Project deleted: " + args[0], GREEN);
            } else {
                console->addLine("Failed to delete project: " + args[0], RED);
            }*/
        }, "Delete a project", "Project",
        "project.delete <name>", deleteParams);

    // scene.list command
    processor->registerCommand("scene.list",
        [console, projectManager](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project is currently open", YELLOW);
                return;
            }

            // Get list of scenes
            std::vector<std::string> scenes;
            std::string scenesPath = projectManager->getCurrentProject()->getPath() + "/scenes";
            if (std::filesystem::exists(scenesPath)) {
                for (const auto& entry : std::filesystem::directory_iterator(scenesPath)) {
                    if (entry.path().extension() == ".json") {
                        scenes.push_back(entry.path().stem().string());
                    }
                }
            }

            if (scenes.empty()) {
                console->addLine("No scenes found in current project", YELLOW);
                console->addLine("Use 'scene.save <name>' to save the current scene", GRAY);
            } else {
                console->addLine("Available scenes:", YELLOW);
                for (const auto& scene : scenes) {
                    console->addLine("  - " + scene, WHITE);
                }
            }
        }, "List all available scenes in the current project", "Scene");
}

} // namespace GameEngine
