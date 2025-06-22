#pragma once

#include <memory>

// Forward declarations
class RenderSystem;
class ResourceManager;
class Console;
class CommandProcessor;
class ScriptManager;
class GameLogicManager;
class Scene;

namespace GameEngine {
    class ProjectManager;
    class BuildSystem;
    class AsyncBuildSystem;
    class PlayMode;
    class SceneSerializer;
    class ComponentRegistry;
    class PackageManager;
    class PackageLoader;
    class PluginManager;
}

namespace GameEngine {
class SystemsManager {
public:
    SystemsManager();
    ~SystemsManager();

    // Initialize all systems
    bool initialize(bool headlessMode);

    // Shutdown all systems
    void shutdown();

    // System getters
    RenderSystem* getRenderSystem() const { return renderSystem.get(); }
    ResourceManager* getResourceManager() const { return resourceManager.get(); }
    Console* getConsole() const { return console.get(); }
    CommandProcessor* getCommandProcessor() const { return commandProcessor.get(); }
    ScriptManager* getScriptManager() const { return scriptManager.get(); }
    GameLogicManager* getGameLogicManager() const { return gameLogicManager.get(); }
    ProjectManager* getProjectManager() const { return projectManager.get(); }
    BuildSystem* getBuildSystem() const { return buildSystem.get(); }
    AsyncBuildSystem* getAsyncBuildSystem() const { return asyncBuildSystem.get(); }
    PlayMode* getPlayMode() const { return playMode.get(); }
    PackageManager* getPackageManager() const { return packageManager.get(); }
    PackageLoader* getPackageLoader() const { return packageLoader.get(); }
    PluginManager* getPluginManager() const { return pluginManager.get(); }

    // Special initialization for components
    void registerComponents();

private:
    // Initialize individual systems
    bool initializeResourceManager(bool headlessMode);
    bool initializeRenderSystem(bool headlessMode);
    bool initializeConsole();
    bool initializeScriptManager();
    bool initializeGameLogicManager();
    bool initializeProjectManager();
    bool initializeBuildSystems();
    bool initializePlayMode();
    bool initializePackageManager();

    // System instances
    std::unique_ptr<RenderSystem> renderSystem;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<Console> console;
    std::unique_ptr<CommandProcessor> commandProcessor;
    std::unique_ptr<ScriptManager> scriptManager;
    std::unique_ptr<GameLogicManager> gameLogicManager;
    std::unique_ptr<ProjectManager> projectManager;
    std::unique_ptr<BuildSystem> buildSystem;
    std::unique_ptr<AsyncBuildSystem> asyncBuildSystem;
    std::unique_ptr<PlayMode> playMode;
    std::unique_ptr<PackageManager> packageManager;
    std::unique_ptr<PackageLoader> packageLoader;
    std::unique_ptr<PluginManager> pluginManager;

    bool headlessMode = false;
};

} // namespace GameEngine
