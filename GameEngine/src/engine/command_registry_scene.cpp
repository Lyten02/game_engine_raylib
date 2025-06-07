#include "command_registry.h"
#include "../console/console.h"
#include "../console/command_processor.h"
#include "../scene/scene.h"
#include "../serialization/scene_serializer.h"
#include "../components/transform.h"
#include "../components/sprite.h"
#include "play_mode.h"
#include "../project/project_manager.h"
#include "../project/project.h"
#include <entt/entt.hpp>
#include <sstream>
#include <filesystem>

namespace GameEngine {

void CommandRegistry::registerSceneCommands(CommandProcessor* processor, Console* console, Scene** currentScene) {
    // scene.create command
    processor->registerCommand("scene.create",
        [console, currentScene](const std::vector<std::string>& args) {
            if (!*currentScene) {
                console->addLine("No active scene to replace", RED);
                return;
            }
            
            // Clear current scene
            (*currentScene)->registry.clear();
            console->addLine("New scene created", GREEN);
        }, "Create a new empty scene", "Scene");
    
    // scene.save command
    {
        std::vector<CommandParameter> saveParams = {
            {"filename", "Scene filename (without extension)", true}
        };
        processor->registerCommand("scene.save",
        [console, currentScene](const std::vector<std::string>& args) {
            if (!*currentScene) {
                console->addLine("No active scene to save", RED);
                return;
            }
            
            if (args.empty()) {
                console->addLine("Usage: scene.save <filename>", RED);
                return;
            }
            
            // Create scenes directory if it doesn't exist
            std::filesystem::create_directories("scenes");
            
            std::string filename = "scenes/" + args[0] + ".json";
            SceneSerializer serializer;
            if (serializer.saveScene(*currentScene, filename)) {
                console->addLine("Scene saved to: " + filename, GREEN);
            } else {
                console->addLine("Failed to save scene", RED);
            }
        }, "Save current scene to file", "Scene", "", saveParams);
    }
    
    // scene.load command
    {
        std::vector<CommandParameter> loadParams = {
            {"filename", "Scene filename (without extension)", true}
        };
        processor->registerCommand("scene.load",
        [console, currentScene](const std::vector<std::string>& args) {
            if (!*currentScene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            if (args.empty()) {
                console->addLine("Usage: scene.load <filename>", RED);
                return;
            }
            
            std::string filename = "scenes/" + args[0] + ".json";
            SceneSerializer serializer;
            if (serializer.loadScene(*currentScene, filename)) {
                console->addLine("Scene loaded from: " + filename, GREEN);
            } else {
                console->addLine("Failed to load scene: " + filename, RED);
            }
        }, "Load scene from file", "Scene", "", loadParams);
    }
    
    // scene.info command
    processor->registerCommand("scene.info",
        [console, currentScene](const std::vector<std::string>& args) {
            if (!*currentScene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            auto& registry = (*currentScene)->registry;
            size_t entityCount = 0;
            // Count entities manually
            entityCount = 0;
            auto view = registry.view<entt::entity>();
            for (auto entity : view) {
                entityCount++;
            }
            
            std::stringstream ss;
            ss << "Scene Information:\n";
            ss << "  Total entities: " << entityCount << "\n";
            ss << "  Components:\n";
            
            auto transformView = registry.view<TransformComponent>();
            ss << "    Transform: " << transformView.size() << "\n";
            
            auto spriteView = registry.view<Sprite>();
            ss << "    Sprite: " << spriteView.size() << "\n";
            
            console->addLine(ss.str(), YELLOW);
        }, "Display current scene information", "Scene");
}

void CommandRegistry::registerScriptCommands(CommandProcessor* processor, Console* console, ScriptManager* scriptManager) {
    if (!scriptManager) return;
    
    // script.run command
    processor->registerCommand("script.run",
        [console, scriptManager](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: script.run <filename>", RED);
                return;
            }
            
            // Script loading not implemented yet
            console->addLine("Script loading not implemented yet", YELLOW);
        }, "Run a Lua script", "Script");
    
    // script.reload command
    processor->registerCommand("script.reload",
        [console, scriptManager](const std::vector<std::string>& args) {
            // Check if script manager has a loaded script
            if (args.empty()) {
                console->addLine("No script currently loaded", RED);
                return;
            }
            
            console->addLine("Script reload not implemented yet", YELLOW);
        }, "Reload the current script", "Script");
}

void CommandRegistry::registerPlayModeCommands(CommandProcessor* processor, Console* console, 
                                             Scene** currentScene, ProjectManager* projectManager, 
                                             PlayMode* playMode) {
    // play command
    processor->registerCommand("play",
        [console, currentScene, projectManager, playMode](const std::vector<std::string>& args) {
            if (!projectManager || !projectManager->getCurrentProject()) {
                console->addLine("No active project to play", RED);
                return;
            }
            
            // Save current scene if modified
            if (*currentScene) {
                SceneSerializer serializer;
                std::string scenePath = projectManager->getCurrentProject()->getPath() + "/scenes/current.json";
                if (serializer.saveScene(*currentScene, scenePath)) {
                    console->addLine("Scene saved before play mode", GRAY);
                }
            }
            
            if (playMode->start(*currentScene, projectManager->getCurrentProject())) {
                console->addLine("Play mode started", GREEN);
            } else {
                console->addLine("Failed to start play mode", RED);
            }
        }, "Start play mode", "Play");
    
    // stop command
    processor->registerCommand("stop",
        [console, playMode](const std::vector<std::string>& args) {
            if (playMode->isStopped()) {
                console->addLine("Play mode is not running", YELLOW);
                return;
            }
            
            playMode->stop();
            console->addLine("Play mode stopped", GREEN);
        }, "Stop play mode", "Play");
    
    // play.status command
    processor->registerCommand("play.status",
        [console, playMode](const std::vector<std::string>& args) {
            if (!playMode->isStopped()) {
                console->addLine("Play mode is running", GREEN);
                // Process ID not available in current API
                // console->addLine("Process ID: " + std::to_string(playMode->getProcessId()), GRAY);
            } else {
                console->addLine("Play mode is not running", YELLOW);
            }
        }, "Check play mode status", "Play");
}

} // namespace GameEngine