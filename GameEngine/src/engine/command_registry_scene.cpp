#include "command_registry.h"
#include "../console/console.h"
#include "../console/command_processor.h"
#include "../scene/scene.h"
#include "../serialization/scene_serializer.h"
// Component headers removed - components are now optional
#include "play_mode.h"
#include "../project/project_manager.h"
#include "../project/project.h"
#include "../scripting/game_logic_manager.h"
#include "../scripting/game_logic_interface.h"
#include <entt/entt.hpp>
#include <sstream>
#include <filesystem>

namespace GameEngine {

void CommandRegistry::registerSceneCommands(CommandProcessor* processor, Console* console, std::function<Scene*()> getScene, ProjectManager* projectManager) {
    // scene.create command
    {
        std::vector<CommandParameter> createParams = {
            {"name", "Name of the scene (optional)", false}
        };
        processor->registerCommand("scene.create",
        [console, getScene](const std::vector<std::string>& args) {
            Scene* scene = getScene();
            if (!scene) {
                console->addLine("No active scene to replace", RED);
                return;
            }
            
            // Clear current scene
            scene->registry.clear();
            
            if (!args.empty()) {
                console->addLine("New scene created: " + args[0], GREEN);
                console->addLine("Use 'scene.save " + args[0] + "' to save it", GRAY);
            } else {
                console->addLine("New scene created", GREEN);
            }
        }, "Create a new empty scene", "Scene", "scene.create [name]", createParams);
    }
    
    // scene.save command
    {
        std::vector<CommandParameter> saveParams = {
            {"filename", "Scene filename (without extension)", true}
        };
        processor->registerCommand("scene.save",
        [console, getScene, projectManager](const std::vector<std::string>& args) {
            Scene* scene = getScene();
            if (!scene) {
                console->addLine("No active scene to save", RED);
                return;
            }
            
            if (args.empty()) {
                console->addLine("Usage: scene.save <filename>", RED);
                return;
            }
            
            if (!projectManager || !projectManager->getCurrentProject()) {
                console->addLine("No project open. Use 'project.open' first.", RED);
                return;
            }
            
            // Create scenes directory in project if it doesn't exist
            std::string scenesDir = projectManager->getCurrentProject()->getPath() + "/scenes";
            std::filesystem::create_directories(scenesDir);
            
            std::string filename = scenesDir + "/" + args[0] + ".json";
            SceneSerializer serializer;
            if (serializer.saveScene(scene, filename)) {
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
        [console, getScene, projectManager](const std::vector<std::string>& args) {
            Scene* scene = getScene();
            if (!scene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            if (args.empty()) {
                console->addLine("Usage: scene.load <filename>", RED);
                return;
            }
            
            if (!projectManager || !projectManager->getCurrentProject()) {
                console->addLine("No project open. Use 'project.open' first.", RED);
                return;
            }
            
            std::string filename = projectManager->getCurrentProject()->getPath() + "/scenes/" + args[0] + ".json";
            SceneSerializer serializer;
            if (serializer.loadScene(scene, filename)) {
                console->addLine("Scene loaded from: " + filename, GREEN);
            } else {
                console->addLine("Failed to load scene: " + filename, RED);
            }
        }, "Load scene from file", "Scene", "", loadParams);
    }
    
    // scene.info command
    processor->registerCommand("scene.info",
        [console, getScene](const std::vector<std::string>& args) {
            Scene* scene = getScene();
            if (!scene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            auto& registry = scene->registry;
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
            // Component counting removed - components are now plugin-based
            // TODO: Add dynamic component statistics via plugin API
            
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
                                             std::function<Scene*()> getScene, ProjectManager* projectManager, 
                                             PlayMode* playMode) {
    // play command
    processor->registerCommand("play",
        [console, getScene, projectManager, playMode](const std::vector<std::string>& args) {
            if (!projectManager || !projectManager->getCurrentProject()) {
                console->addLine("No active project to play", RED);
                return;
            }
            
            // Save current scene if modified
            Scene* scene = getScene();
            if (scene) {
                SceneSerializer serializer;
                std::string scenePath = projectManager->getCurrentProject()->getPath() + "/scenes/current.json";
                if (serializer.saveScene(scene, scenePath)) {
                    console->addLine("Scene saved before play mode", GRAY);
                }
            }
            
            if (playMode->start(scene, projectManager->getCurrentProject())) {
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

// Forward declaration for ExampleGameLogic
class ExampleGameLogic : public IGameLogic {
public:
    void initialize(entt::registry& registry) override {}
    void update(entt::registry& registry, float deltaTime, const InputState& input) override {
        // Example logic - components are now plugin-based
        // Plugins should register their own components and update logic
    }
    void shutdown() override {}
    std::string getName() const override { return "ExampleGameLogic"; }
};

void CommandRegistry::registerGameLogicCommands(CommandProcessor* processor, Console* console, 
                                              GameLogicManager* gameLogicManager, std::function<Scene*()> getScene) {
    if (!gameLogicManager) return;
    
    // logic.list command
    processor->registerCommand("logic.list",
        [console, gameLogicManager](const std::vector<std::string>& args) {
            auto activeLogics = gameLogicManager->getActiveLogics();
            
            if (activeLogics.empty()) {
                console->addLine("No active game logics", YELLOW);
            } else {
                console->addLine("Active game logics:", GREEN);
                for (const auto& logic : activeLogics) {
                    console->addLine("  - " + logic, GRAY);
                }
            }
        }, "List active game logics", "GameLogic");
    
    // logic.create command
    {
        std::vector<CommandParameter> createParams = {
            {"name", "Name of the game logic to create", true}
        };
        processor->registerCommand("logic.create",
            [console, gameLogicManager, getScene](const std::vector<std::string>& args) {
                if (args.empty()) {
                    console->addLine("Usage: logic.create <name>", RED);
                    return;
                }
                
                Scene* scene = getScene();
                if (!scene) {
                    console->addLine("No active scene for game logic", RED);
                    return;
                }
                
                if (gameLogicManager->createLogic(args[0], scene->registry)) {
                    console->addLine("Created game logic: " + args[0], GREEN);
                } else {
                    console->addLine("Failed to create game logic: " + args[0], RED);
                    console->addLine("Make sure the logic is registered", GRAY);
                }
            }, "Create a new game logic instance", "GameLogic", "", createParams);
    }
    
    // logic.remove command
    {
        std::vector<CommandParameter> removeParams = {
            {"name", "Name of the game logic to remove", true}
        };
        processor->registerCommand("logic.remove",
            [console, gameLogicManager](const std::vector<std::string>& args) {
                if (args.empty()) {
                    console->addLine("Usage: logic.remove <name>", RED);
                    return;
                }
                
                if (gameLogicManager->removeLogic(args[0])) {
                    console->addLine("Removed game logic: " + args[0], GREEN);
                } else {
                    console->addLine("Failed to remove game logic: " + args[0], RED);
                    console->addLine("Logic not found or already removed", GRAY);
                }
            }, "Remove a game logic instance", "GameLogic", "", removeParams);
    }
    
    // logic.clear command
    processor->registerCommand("logic.clear",
        [console, gameLogicManager](const std::vector<std::string>& args) {
            gameLogicManager->clearLogics();
            console->addLine("All game logics cleared", GREEN);
        }, "Remove all active game logics", "GameLogic");
    
    // logic.register command - Example registration
    processor->registerCommand("logic.register.example",
        [console, gameLogicManager](const std::vector<std::string>& args) {
            // Register the example game logic factory
            auto exampleFactory = []() -> std::unique_ptr<IGameLogic> {
                return std::make_unique<ExampleGameLogic>();
            };
            gameLogicManager->registerLogicFactory("ExampleGameLogic", exampleFactory);
            console->addLine("Registered ExampleGameLogic", GREEN);
            console->addLine("Use 'logic.create ExampleGameLogic' to instantiate", GRAY);
        }, "Register the example game logic", "GameLogic");
}

} // namespace GameEngine