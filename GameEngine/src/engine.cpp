#include "engine.h"
#include "engine/engine_core.h"
#include "engine/systems_manager.h"
#include "engine/command_registry.h"
#include "scene/scene.h"
#include "console/console.h"
#include "resources/resource_manager.h"
#include "systems/render_system.h"
#include "scripting/script_manager.h"
#include "scripting/game_logic_manager.h"
#include "project/project_manager.h"
#include "project/project.h"
#include "build/async_build_system.h"
#include "engine/play_mode.h"
#include <raylib.h>
#include <spdlog/spdlog.h>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>


Engine::Engine() = default;
Engine::~Engine() {
    cleanup();
}

bool Engine::initialize() {
    try {
        // Create core modules with nullptr checks
        engineCore = std::make_unique<GameEngine::EngineCore>();
        if (!engineCore) {
            spdlog::error("Engine::initialize - Failed to create EngineCore");
            return false;
        }
        
        systemsManager = std::make_unique<GameEngine::SystemsManager>();
        if (!systemsManager) {
            spdlog::error("Engine::initialize - Failed to create SystemsManager");
            cleanup();
            return false;
        }
        
        commandRegistry = std::make_unique<GameEngine::CommandRegistry>();
        if (!commandRegistry) {
            spdlog::error("Engine::initialize - Failed to create CommandRegistry");
            cleanup();
            return false;
        }
        
        // Initialize engine core
        if (!engineCore->initialize(headlessMode)) {
            spdlog::error("Engine::initialize - Failed to initialize engine core");
            cleanup();
            return false;
        }
        
        // Initialize all systems
        if (!systemsManager->initialize(headlessMode)) {
            spdlog::error("Engine::initialize - Failed to initialize systems");
            cleanup();
            return false;
        }
        
        // Register all commands
        commandRegistry->registerAllCommands(
            systemsManager->getCommandProcessor(),
            engineCore.get(),
            systemsManager->getConsole(),
            [this]() -> Scene* { return currentScene.get(); },
            systemsManager->getResourceManager(),
            systemsManager->getScriptManager(),
            systemsManager->getGameLogicManager(),
            systemsManager->getProjectManager(),
            systemsManager->getBuildSystem(),
            systemsManager->getAsyncBuildSystem(),
            systemsManager->getPlayMode(),
            systemsManager->getPackageManager(),
            this
        );
        
        // Create default scene 
        createScene();
        
        if (!headlessMode) {
            spdlog::info("Engine::initialize - Engine initialized in project management mode");
        }
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Engine::initialize - Exception during initialization: {}", e.what());
        cleanup();
        return false;
    } catch (...) {
        spdlog::error("Engine::initialize - Unknown exception during initialization");
        cleanup();
        return false;
    }
}

void Engine::run() {
    if (!engineCore->isRunning()) {
        spdlog::warn("Engine::run - Engine not initialized, aborting run");
        return;
    }
    
    // In headless mode, run a simplified game loop
    if (headlessMode) {
        spdlog::info("Engine::run - Starting headless game loop");
        runHeadless();
        return;
    }
    
    spdlog::info("Engine::run - Starting main game loop");
    
    auto* console = systemsManager->getConsole();
    auto* playMode = systemsManager->getPlayMode();
    auto* asyncBuildSystem = systemsManager->getAsyncBuildSystem();
    auto* renderSystem = systemsManager->getRenderSystem();
    auto* projectManager = systemsManager->getProjectManager();
    auto* gameLogicManager = systemsManager->getGameLogicManager();
    
    // Main game loop
    while (engineCore->shouldContinueRunning()) {
        // Update phase
        float deltaTime = engineCore->getFrameTime();
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
            
            // Update game logic manager
            if (gameLogicManager) {
                gameLogicManager->update(currentScene->registry, deltaTime);
            }
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

GameLogicManager* Engine::getGameLogicManager() const {
    return systemsManager ? systemsManager->getGameLogicManager() : nullptr;
}

GameEngine::ProjectManager* Engine::getProjectManager() const {
    return systemsManager ? systemsManager->getProjectManager() : nullptr;
}

void Engine::createScene() {
    if (!currentScene) {
        currentScene = std::make_unique<Scene>();
        currentScene->onCreate();
    }
}

void Engine::destroyScene() {
    if (currentScene) {
        currentScene->onDestroy();
        currentScene.reset();
    }
}

void Engine::runHeadless() {
    spdlog::info("Engine::runHeadless - Starting headless game loop");
    
    auto* console = systemsManager->getConsole();
    auto* projectManager = systemsManager->getProjectManager();
    auto* asyncBuildSystem = systemsManager->getAsyncBuildSystem();
    
    // Configuration for headless mode
    auto startTime = std::chrono::steady_clock::now();
    const auto maxDuration = std::chrono::seconds(300); // 5 minutes max runtime
    const float targetDeltaTime = 1.0f / 60.0f; // 60 FPS equivalent
    
    // Main headless loop
    int idleFrames = 0; // Move outside static scope
    
    while (engineCore->isRunning()) {
        auto frameStart = std::chrono::steady_clock::now();
        
        // Check timeout
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);
        
        if (elapsedTime > maxDuration) {
            spdlog::info("Engine::runHeadless - Maximum runtime reached ({}s), shutting down", elapsedTime.count());
            break;
        }
        
        // Update engine core
        engineCore->processFrame(targetDeltaTime);
        
        // Update console for command processing
        if (console) {
            console->update(targetDeltaTime);
        }
        
        // Update current scene if project is loaded
        if (currentScene && projectManager && projectManager->getCurrentProject()) {
            currentScene->onUpdate(targetDeltaTime);
        }
        
        // Check if async build system has pending operations
        bool hasPendingOperations = false;
        if (asyncBuildSystem) {
            auto status = asyncBuildSystem->getStatus();
            hasPendingOperations = (status == GameEngine::AsyncBuildSystem::BuildStatus::InProgress);
        }
        
        // Auto-exit if no operations are pending
        if (!hasPendingOperations) {
            idleFrames++;
            
            // Wait for 60 frames (1 second) of idle time before exiting
            if (idleFrames > 60) {
                spdlog::info("Engine::runHeadless - No pending operations, auto-exiting after {} frames", idleFrames);
                break;
            }
        } else {
            // Reset idle counter if there's activity
            idleFrames = 0;
        }
        
        // Frame timing to maintain consistent update rate
        auto frameEnd = std::chrono::steady_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        auto targetFrameDuration = std::chrono::milliseconds(16); // ~60 FPS
        
        if (frameDuration < targetFrameDuration) {
            std::this_thread::sleep_for(targetFrameDuration - frameDuration);
        }
    }
    
    spdlog::info("Engine::runHeadless - Headless game loop ended");
}

void Engine::cleanup() {
    // Reset in reverse order of creation
    commandRegistry.reset();
    systemsManager.reset();
    engineCore.reset();
    currentScene.reset();
}