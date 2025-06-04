#include "engine.h"
#include <raylib.h>
#include <spdlog/spdlog.h>
#include "systems/render_system.h"
#include "scene/scene.h"
#include "components/transform.h"
#include "components/sprite.h"

Engine::Engine() = default;
Engine::~Engine() = default;

bool Engine::initialize(int width, int height, const std::string& title) {
    spdlog::info("Engine::initialize - Starting engine initialization");
    
    // Initialize Raylib window
    InitWindow(width, height, title.c_str());
    
    if (!IsWindowReady()) {
        spdlog::error("Engine::initialize - Failed to create window");
        return false;
    }
    
    // Set target FPS
    SetTargetFPS(60);
    
    // Initialize spdlog
    spdlog::set_level(spdlog::level::debug);
    
    // Initialize ECS systems
    renderSystem = std::make_unique<RenderSystem>();
    renderSystem->initialize();
    spdlog::info("Engine::initialize - Render system created and initialized");
    
    // Create a test scene
    currentScene = std::make_unique<Scene>();
    currentScene->onCreate();
    
    // Create a test entity with Transform and Sprite components
    auto testEntity = currentScene->registry.create();
    currentScene->registry.emplace<TransformComponent>(testEntity, 
        TransformComponent{
            Vector3{640.0f, 360.0f, 0.0f},    // Position at center
            Vector3{0.0f, 0.0f, 0.0f},        // No rotation
            Vector3{1.0f, 1.0f, 1.0f}         // Normal scale
        }
    );
    
    // Create a placeholder sprite component
    Sprite& sprite = currentScene->registry.emplace<Sprite>(testEntity);
    sprite.tint = WHITE;
    // Note: texture will be empty for now, which is fine for testing
    
    spdlog::info("Engine::initialize - Test scene created with {} entity", 1);
    
    running = true;
    spdlog::info("Engine::initialize - Engine initialized successfully ({}x{}, \"{}\")", 
                 width, height, title);
    
    return true;
}

void Engine::run() {
    spdlog::info("Engine::run - Starting main game loop");
    
    if (!running) {
        spdlog::warn("Engine::run - Engine not initialized, aborting run");
        return;
    }
    
    // Main game loop
    while (running && !WindowShouldClose()) {
        // Update phase
        float deltaTime = GetFrameTime();
        
        // Update the current scene
        if (currentScene) {
            currentScene->onUpdate(deltaTime);
        }
        
        // Draw phase
        BeginDrawing();
        ClearBackground(GRAY);
        
        // Update render system with current scene's registry
        if (renderSystem && currentScene) {
            renderSystem->update(currentScene->registry);
        }
        
        EndDrawing();
    }
    
    spdlog::info("Engine::run - Main game loop ended");
}

void Engine::shutdown() {
    spdlog::info("Engine::shutdown - Shutting down engine");
    
    // Cleanup scene
    if (currentScene) {
        currentScene->onDestroy();
        currentScene.reset();
        spdlog::info("Engine::shutdown - Scene destroyed");
    }
    
    // Cleanup systems
    if (renderSystem) {
        renderSystem->shutdown();
        renderSystem.reset();
        spdlog::info("Engine::shutdown - Render system shut down");
    }
    
    if (IsWindowReady()) {
        CloseWindow();
        spdlog::info("Engine::shutdown - Window closed");
    }
    
    running = false;
    spdlog::info("Engine::shutdown - Engine shutdown complete");
}