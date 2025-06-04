#pragma once

#include <string>
#include <memory>
#include <entt/entt.hpp>

class RenderSystem;
class ResourceManager;

namespace GameEngine {

class GameRuntime {
private:
    bool running = false;
    entt::registry registry;
    std::unique_ptr<RenderSystem> renderSystem;
    std::unique_ptr<ResourceManager> resourceManager;
    std::string currentScenePath;
    
public:
    GameRuntime();
    ~GameRuntime();
    
    bool initialize(const std::string& gameConfigPath);
    void run();
    void shutdown();
    void loadScene(const std::string& scenePath);
    
private:
    void update(float deltaTime);
    void render();
    bool loadGameConfig(const std::string& configPath);
};

} // namespace GameEngine