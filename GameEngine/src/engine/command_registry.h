#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>

// Forward declarations
class Console;
class CommandProcessor;
class Scene;
class ResourceManager;
class ScriptManager;
class GameLogicManager;
class Engine;  // Add global Engine forward declaration
namespace GameEngine {
    class ProjectManager;
    class BuildSystem;
    class AsyncBuildSystem;
    class PlayMode;
    class EngineCore;
    class PackageManager;
}

namespace GameEngine {

class CommandRegistry {
public:
    CommandRegistry();
    ~CommandRegistry();

    // Register all commands with the processor
    void registerAllCommands(CommandProcessor* processor,
                           EngineCore* engineCore,
                           Console* console,
                           std::function<Scene*()> getScene,
                           ResourceManager* resourceManager,
                           ScriptManager* scriptManager,
                           GameLogicManager* gameLogicManager,
                           ProjectManager* projectManager,
                           BuildSystem* buildSystem,
                           AsyncBuildSystem* asyncBuildSystem,
                           PlayMode* playMode,
                           PackageManager* packageManager,
                           Engine* engine);

private:
    // Command registration methods grouped by category
    void registerEngineCommands(CommandProcessor* processor, EngineCore* engineCore, Console* console);
    void registerSceneCommands(CommandProcessor* processor, Console* console, std::function<Scene*()> getScene, ProjectManager* projectManager);
    void registerEntityCommands(CommandProcessor* processor, Console* console, std::function<Scene*()> getScene, ResourceManager* resourceManager);
    void registerResourceCommands(CommandProcessor* processor, Console* console);
    void registerRenderCommands(CommandProcessor* processor, Console* console, std::function<Scene*()> getScene);
    void registerDebugCommands(CommandProcessor* processor, Console* console, bool* showDebugInfo);
    void registerConsoleCommands(CommandProcessor* processor, Console* console);
    void registerConfigCommands(CommandProcessor* processor, Console* console, EngineCore* engineCore);
    void registerScriptCommands(CommandProcessor* processor, Console* console, ScriptManager* scriptManager);
    void registerGameLogicCommands(CommandProcessor* processor, Console* console, GameLogicManager* gameLogicManager, std::function<Scene*()> getScene);
    void registerProjectCommands(CommandProcessor* processor, Console* console, ProjectManager* projectManager, std::function<Scene*()> getScene, Engine* engine);
    void registerBuildCommands(CommandProcessor* processor, Console* console, ProjectManager* projectManager, 
                             BuildSystem* buildSystem, AsyncBuildSystem* asyncBuildSystem);
    void registerPlayModeCommands(CommandProcessor* processor, Console* console, std::function<Scene*()> getScene,
                                ProjectManager* projectManager, PlayMode* playMode);
    void registerLogCommands(CommandProcessor* processor, Console* console);
    void registerPackageCommands(CommandProcessor* processor, Console* console, PackageManager* packageManager);

    // Helper methods for suggestion providers
    std::vector<std::string> getSceneList(ProjectManager* projectManager) const;
    std::vector<std::string> getProjectList() const;
    std::vector<std::string> getScriptList() const;
    std::vector<std::string> getConfigKeys() const;
    
    // State variables managed by commands
    bool* showDebugInfo = nullptr;
};

} // namespace GameEngine