#include "engine.h"
#include "engine/engine_core.h"
#include "engine/systems_manager.h"
#include "engine/command_registry.h"
#include "scene/scene.h"
#include "console/console.h"
#include "resources/resource_manager.h"
#include "systems/render_system.h"
#include "scripting/script_manager.h"
#include "project/project_manager.h"
#include "project/project.h"
#include "build/async_build_system.h"
#include "engine/play_mode.h"
#include <raylib.h>
#include <spdlog/spdlog.h>
#include <iomanip>
#include <sstream>


Engine::Engine() = default;
Engine::~Engine() = default;

bool Engine::initialize() {
    // Create core modules
    engineCore = std::make_unique<GameEngine::EngineCore>();
    systemsManager = std::make_unique<GameEngine::SystemsManager>();
    commandRegistry = std::make_unique<GameEngine::CommandRegistry>();
    
    // Initialize engine core
    if (!engineCore->initialize(headlessMode)) {
        spdlog::error("Engine::initialize - Failed to initialize engine core");
        return false;
    }
    
    // Initialize all systems
    if (!systemsManager->initialize(headlessMode)) {
        spdlog::error("Engine::initialize - Failed to initialize systems");
        return false;
    }
    
    // Register all commands
    commandRegistry->registerAllCommands(
        systemsManager->getCommandProcessor(),
        engineCore.get(),
        systemsManager->getConsole(),
        &currentScenePtr,
        systemsManager->getResourceManager(),
        systemsManager->getScriptManager(),
        systemsManager->getProjectManager(),
        systemsManager->getBuildSystem(),
        systemsManager->getAsyncBuildSystem(),
        systemsManager->getPlayMode(),
        this
    );
    
    // Create default scene 
    createScene();
    
    if (!headlessMode) {
        spdlog::info("Engine::initialize - Engine initialized in project management mode");
    }
    
    return true;
}

void Engine::run() {
    if (!engineCore->isRunning()) {
        spdlog::warn("Engine::run - Engine not initialized, aborting run");
        return;
    }
    
    // In headless mode, we don't run a game loop
    if (headlessMode) {
        spdlog::info("Engine::run - Headless mode, skipping game loop");
        return;
    }
    
    spdlog::info("Engine::run - Starting main game loop");
    
    auto* console = systemsManager->getConsole();
    auto* playMode = systemsManager->getPlayMode();
    auto* asyncBuildSystem = systemsManager->getAsyncBuildSystem();
    auto* renderSystem = systemsManager->getRenderSystem();
    auto* projectManager = systemsManager->getProjectManager();
    
    // Main game loop
    while (engineCore->shouldContinueRunning()) {
        // Update phase
        float deltaTime = GetFrameTime();
        engineCore->processFrame(deltaTime);
        
        // Update console
        if (console) {
            console->update(deltaTime);
        }
        
        // Update play mode if active
        if (playMode && playMode->isPlaying()) {
            playMode->update(deltaTime);
        }
        // Otherwise update the current scene only if a project is loaded
        else if (currentScene && !console->isOpen() && projectManager && projectManager->getCurrentProject()) {
            currentScene->onUpdate(deltaTime);
        }
        
        // Handle play mode controls
        if (IsKeyPressed(KEY_F5)) {
            if (playMode->isPlaying() || playMode->isPaused()) {
                playMode->stop();
                console->addLine("Play mode stopped", YELLOW);
            } else if (currentScene && projectManager && projectManager->getCurrentProject()) {
                if (playMode->start(currentScene.get(), projectManager->getCurrentProject())) {
                    console->addLine("Play mode started - Press F5 to stop, F6 to pause", GREEN);
                } else {
                    console->addLine("Failed to start play mode", RED);
                }
            }
        }
        
        if (IsKeyPressed(KEY_F6) && playMode) {
            if (playMode->isPlaying()) {
                playMode->pause();
                console->addLine("Play mode paused", YELLOW);
            } else if (playMode->isPaused()) {
                playMode->resume();
                console->addLine("Play mode resumed", GREEN);
            }
        }
        
        // Check for async build messages
        if (asyncBuildSystem && asyncBuildSystem->getStatus() == GameEngine::AsyncBuildSystem::BuildStatus::InProgress) {
            while (asyncBuildSystem->hasMessages()) {
                std::string msg = asyncBuildSystem->getNextMessage();
                if (!msg.empty()) {
                    console->addLine(msg, GRAY);
                }
            }
        }
        
        // Draw phase
        engineCore->beginFrame();
        engineCore->clearBackground();
        
        // Update render system with appropriate scene
        if (renderSystem) {
            if (playMode && !playMode->isStopped() && playMode->getPlayScene()) {
                // Render play scene when in play mode
                renderSystem->update(playMode->getPlayScene()->registry);
            } else if (currentScene) {
                // Render editor scene
                renderSystem->update(currentScene->registry);
            }
        }
        
        // Render play mode UI
        if (playMode && !playMode->isStopped()) {
            playMode->renderUI(console);
        }
        
        // Render console on top
        if (console) {
            console->render();
        }
        
        // Draw help text if console is not open
        if (console && !console->isOpen()) {
            DrawText("Press F1 to open console", 10, 10, 20, LIGHTGRAY);
            
            // Show build progress if building
            if (asyncBuildSystem && asyncBuildSystem->getStatus() == GameEngine::AsyncBuildSystem::BuildStatus::InProgress) {
                float progress = asyncBuildSystem->getProgress();
                std::string status = asyncBuildSystem->getCurrentStep();
                
                int barWidth = 400;
                int barHeight = 20;
                int barX = (GetScreenWidth() - barWidth) / 2;
                int barY = GetScreenHeight() / 2;
                
                // Draw progress bar background
                DrawRectangle(barX - 2, barY - 2, barWidth + 4, barHeight + 4, BLACK);
                DrawRectangle(barX, barY, barWidth, barHeight, DARKGRAY);
                
                // Draw progress
                DrawRectangle(barX, barY, (int)(barWidth * progress), barHeight, GREEN);
                
                // Draw status text
                int textWidth = MeasureText(status.c_str(), 16);
                DrawText(status.c_str(), (GetScreenWidth() - textWidth) / 2, barY - 25, 16, WHITE);
                
                // Draw percentage
                std::string percentText = std::to_string((int)(progress * 100)) + "%";
                int percentWidth = MeasureText(percentText.c_str(), 14);
                DrawText(percentText.c_str(), (GetScreenWidth() - percentWidth) / 2, barY + barHeight + 5, 14, WHITE);
            }
        }
        
        // Draw debug info in bottom right corner
        if (showDebugInfo) {
            int screenWidth = GetScreenWidth();
            int screenHeight = GetScreenHeight();
            
            // FPS
            std::string fpsText = "FPS: " + std::to_string(GetFPS());
            int fpsWidth = MeasureText(fpsText.c_str(), 16);
            DrawText(fpsText.c_str(), screenWidth - fpsWidth - 10, screenHeight - 60, 16, GREEN);
            
            // Frame time
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << GetFrameTime() * 1000.0f << " ms";
            std::string frameTimeText = ss.str();
            int frameTimeWidth = MeasureText(frameTimeText.c_str(), 14);
            DrawText(frameTimeText.c_str(), screenWidth - frameTimeWidth - 10, screenHeight - 40, 14, LIGHTGRAY);
            
            // Entity count
            if (currentScene) {
                int entityCount = 0;
                auto view = currentScene->registry.view<entt::entity>();
                for (auto entity : view) {
                    entityCount++;
                }
                std::string entityText = "Entities: " + std::to_string(entityCount);
                int entityWidth = MeasureText(entityText.c_str(), 14);
                DrawText(entityText.c_str(), screenWidth - entityWidth - 10, screenHeight - 20, 14, LIGHTGRAY);
            }
        }
        
        engineCore->endFrame();
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
    
    // Shutdown systems
    if (systemsManager) {
        systemsManager->shutdown();
        systemsManager.reset();
    }
    
    // Shutdown engine core
    if (engineCore) {
        engineCore->shutdown();
        engineCore.reset();
    }
    
    spdlog::info("Engine::shutdown - Engine shutdown complete");
}

void Engine::requestQuit() {
    if (engineCore) {
        engineCore->requestQuit();
    }
}

// Delegate getters to SystemsManager
RenderSystem* Engine::getRenderSystem() const {
    return systemsManager ? systemsManager->getRenderSystem() : nullptr;
}

ResourceManager* Engine::getResourceManager() const {
    return systemsManager ? systemsManager->getResourceManager() : nullptr;
}

Console* Engine::getConsole() const {
    return systemsManager ? systemsManager->getConsole() : nullptr;
}

CommandProcessor* Engine::getCommandProcessor() const {
    return systemsManager ? systemsManager->getCommandProcessor() : nullptr;
}

ScriptManager* Engine::getScriptManager() const {
    return systemsManager ? systemsManager->getScriptManager() : nullptr;
}

GameEngine::ProjectManager* Engine::getProjectManager() const {
    return systemsManager ? systemsManager->getProjectManager() : nullptr;
}

void Engine::createScene() {
    if (!currentScene) {
        currentScene = std::make_unique<Scene>();
        currentScenePtr = currentScene.get();
        currentScene->onCreate();
    }
}

void Engine::destroyScene() {
    if (currentScene) {
        currentScene->onDestroy();
        currentScene.reset();
        currentScenePtr = nullptr;
    }
}