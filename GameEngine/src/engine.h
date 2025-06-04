#pragma once

#include <string>
#include <memory>
#include <entt/entt.hpp>

// Forward declarations
class RenderSystem;
class Scene;
class ResourceManager;
class Console;
class CommandProcessor;
class ScriptManager;

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
    
    // Getters for systems
    RenderSystem* getRenderSystem() const { return renderSystem.get(); }
    Scene* getCurrentScene() const { return currentScene.get(); }
    ResourceManager* getResourceManager() const { return resourceManager.get(); }
    Console* getConsole() const { return console.get(); }
    CommandProcessor* getCommandProcessor() const { return commandProcessor.get(); }
    ScriptManager* getScriptManager() const { return scriptManager.get(); }
    
    // Engine control
    void requestQuit() { running = false; }
    
private:
    void registerEngineCommands();
    
    bool running = false;
    
    // ECS Systems
    std::unique_ptr<RenderSystem> renderSystem;
    
    // Resource management
    std::unique_ptr<ResourceManager> resourceManager;
    
    // Developer console
    std::unique_ptr<Console> console;
    std::unique_ptr<CommandProcessor> commandProcessor;
    
    // Scripting
    std::unique_ptr<ScriptManager> scriptManager;
    
    // Current active scene
    std::unique_ptr<Scene> currentScene;
    
    // Engine stats
    float totalTime = 0.0f;
    bool showDebugInfo = true;
    bool vsyncEnabled = false;
    int targetFPS = 0;
};