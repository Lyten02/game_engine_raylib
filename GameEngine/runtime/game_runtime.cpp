#include "game_runtime.h"
#include "../src/systems/render_system.h"
#include "../src/resources/resource_manager.h"
#include "../src/components/transform.h"
#include "../src/components/sprite.h"
#include "../src/utils/file_utils.h"
#include <raylib.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <spdlog/spdlog.h>

namespace GameEngine {

GameRuntime::GameRuntime() = default;
GameRuntime::~GameRuntime() = default;

bool GameRuntime::initialize(const std::string& gameConfigPath) {
    spdlog::info("Initializing game runtime");
    
    // Load game configuration
    if (!loadGameConfig(gameConfigPath)) {
        spdlog::error("Failed to load game configuration");
        return false;
    }
    
    // Initialize Raylib window
    InitWindow(800, 600, "Game");
    if (!IsWindowReady()) {
        spdlog::error("Failed to create window");
        return false;
    }
    
    SetTargetFPS(60);
    
    // Initialize resource manager
    resourceManager = std::make_unique<ResourceManager>();
    
    // Initialize render system
    renderSystem = std::make_unique<RenderSystem>();
    renderSystem->initialize();
    
    // Setup 2D camera
    Camera2D camera = {0};
    camera.target = {0.0f, 0.0f};
    camera.offset = {400.0f, 300.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    renderSystem->setCamera2D(camera);
    
    running = true;
    spdlog::info("Game runtime initialized successfully");
    return true;
}

void GameRuntime::run() {
    if (!running) {
        spdlog::error("Game runtime not initialized");
        return;
    }
    
    while (running && !WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        update(deltaTime);
        render();
    }
}

void GameRuntime::shutdown() {
    spdlog::info("Shutting down game runtime");
    
    registry.clear();
    
    if (renderSystem) {
        renderSystem->shutdown();
        renderSystem.reset();
    }
    
    if (resourceManager) {
        resourceManager->unloadAll();
        resourceManager.reset();
    }
    
    if (IsWindowReady()) {
        CloseWindow();
    }
    
    running = false;
}

void GameRuntime::loadScene(const std::string& scenePath) {
    try {
        if (!fileExists(scenePath)) {
            spdlog::error("Scene file not found: {}", scenePath);
            return;
        }
        
        std::string content = FileUtils::readFile(scenePath);
        nlohmann::json sceneJson = nlohmann::json::parse(content);
        
        // Clear existing entities
        registry.clear();
        
        // Load entities from scene
        if (sceneJson.contains("entities") && sceneJson["entities"].is_array()) {
            for (const auto& entityJson : sceneJson["entities"]) {
                entt::entity entity = registry.create();
                
                if (entityJson.contains("components")) {
                    const auto& components = entityJson["components"];
                    
                    // Load Transform component
                    if (components.contains("Transform")) {
                        auto& transform = registry.emplace<TransformComponent>(entity);
                        transform.from_json(components["Transform"]);
                    }
                    
                    // Load Sprite component
                    if (components.contains("Sprite")) {
                        auto& sprite = registry.emplace<Sprite>(entity);
                        sprite.from_json(components["Sprite"]);
                        
                        // Load texture if specified
                        if (!sprite.texturePath.empty()) {
                            sprite.texture = resourceManager->loadTexture(
                                "assets/" + sprite.texturePath, 
                                sprite.texturePath
                            );
                        }
                    }
                }
            }
        }
        
        currentScenePath = scenePath;
        spdlog::info("Scene loaded: {}", scenePath);
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to load scene: {}", e.what());
    }
}

void GameRuntime::update(float deltaTime) {
    // Update game logic here
    // In a full implementation, this would update physics, scripts, etc.
}

void GameRuntime::render() {
    BeginDrawing();
    ClearBackground(DARKGRAY);
    
    // Render all entities
    if (renderSystem) {
        renderSystem->update(registry);
    }
    
    EndDrawing();
}

bool GameRuntime::loadGameConfig(const std::string& configPath) {
    try {
        if (!fileExists(configPath)) {
            spdlog::error("Game config not found: {}", configPath);
            return false;
        }
        
        std::string content = FileUtils::readFile(configPath);
        nlohmann::json config = nlohmann::json::parse(content);
        
        // Load window settings
        if (config.contains("window")) {
            const auto& window = config["window"];
            int width = window.value("width", 800);
            int height = window.value("height", 600);
            std::string title = window.value("title", "Game");
            
            // Update window if needed
            if (IsWindowReady()) {
                SetWindowSize(width, height);
                SetWindowTitle(title.c_str());
            }
        }
        
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to load game config: {}", e.what());
        return false;
    }
}

} // namespace GameEngine