#pragma once

#include <string>
#include <memory>
#include <entt/entt.hpp>

// Forward declarations
class Scene;

namespace GameEngine {
    class EngineCore;
    class SystemsManager;
    class CommandRegistry;
    class ProjectManager;
}

class Engine {
public:
    Engine();
    ~Engine();
    
    // Initialize the engine (loads config and uses those parameters)
    bool initialize();
    
    // Main game loop
    void run();
    
    // Shutdown and cleanup
    void shutdown();
    
    // Headless mode support
    void setHeadlessMode(bool headless) { headlessMode = headless; }
    bool isHeadlessMode() const { return headlessMode; }
    
    // Getters for systems (delegated to SystemsManager)
    class RenderSystem* getRenderSystem() const;
    Scene* getCurrentScene() const { return currentScene.get(); }
    class ResourceManager* getResourceManager() const;
    class Console* getConsole() const;
    class CommandProcessor* getCommandProcessor() const;
    class ScriptManager* getScriptManager() const;
    GameEngine::ProjectManager* getProjectManager() const;
    
    // Engine control
    void requestQuit();
    
    // Debug info
    bool isShowingDebugInfo() const { return showDebugInfo; }
    
    // Scene management for commands
    void createScene();
    void destroyScene();
    
private:
    // Core modules
    std::unique_ptr<GameEngine::EngineCore> engineCore;
    std::unique_ptr<GameEngine::SystemsManager> systemsManager;
    std::unique_ptr<GameEngine::CommandRegistry> commandRegistry;
    
    // Current active scene
    std::unique_ptr<Scene> currentScene;
    
    // Engine state
    bool headlessMode = false;
    bool showDebugInfo = true;
};