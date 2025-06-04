#pragma once

#include <string>
#include <memory>
#include <entt/entt.hpp>

// Forward declarations
class RenderSystem;
class Scene;

class Engine {
public:
    Engine();
    ~Engine();
    
    // Initialize the engine with window parameters
    bool initialize(int width, int height, const std::string& title);
    
    // Main game loop
    void run();
    
    // Shutdown and cleanup
    void shutdown();
    
    // Getters for systems
    RenderSystem* getRenderSystem() const { return renderSystem.get(); }
    Scene* getCurrentScene() const { return currentScene.get(); }
    
private:
    bool running = false;
    
    // ECS Systems
    std::unique_ptr<RenderSystem> renderSystem;
    
    // Current active scene
    std::unique_ptr<Scene> currentScene;
};