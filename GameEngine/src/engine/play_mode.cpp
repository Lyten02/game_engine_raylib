#include "play_mode.h"
#include "../scene/scene.h"
#include "../console/console.h"
#include "../project/project.h"
#include "../serialization/scene_serializer.h"
#include "../components/transform.h"
#include "../components/sprite.h"
#include "../scripting/game_logic_manager.h"
#include <raylib.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <vector>

namespace GameEngine {

bool PlayMode::start(Scene* currentScene, Project* project, GameLogicManager* gameLogicManager) {
    if (state != PlayModeState::Stopped || !currentScene) {
        return false;
    }
    
    try {
        // Store references
        editorScene = currentScene;
        this->gameLogicManager = gameLogicManager;
        
        // Create play scene
        playScene = std::make_unique<Scene>();
        playScene->onCreate();
        
        // Check if we should load the start scene instead
        bool loadStartScene = false;
        std::string startScenePath;
        
        if (project && project->hasStartScene()) {
            std::string startSceneName = project->getStartScene();
            startScenePath = project->getPath() + "/scenes/" + startSceneName + ".json";
            
            if (std::filesystem::exists(startScenePath)) {
                loadStartScene = true;
                spdlog::info("PlayMode: Loading start scene: {}", startSceneName);
            }
        }
        
        if (loadStartScene) {
            // Load the start scene from file
            SceneSerializer serializer;
            if (!serializer.loadScene(playScene.get(), startScenePath)) {
                spdlog::error("PlayMode: Failed to load start scene, using current scene");
                // Fall back to current scene
                nlohmann::json sceneData = SceneSerializer::sceneToJson(currentScene);
                SceneSerializer::jsonToScene(sceneData, playScene.get());
            }
        } else {
            // Use current scene as before
            nlohmann::json sceneData = SceneSerializer::sceneToJson(currentScene);
            SceneSerializer::jsonToScene(sceneData, playScene.get());
        }
        
        // Initialize game logic for play scene
        if (gameLogicManager && playScene && project) {
            // Clear any existing logics
            gameLogicManager->clearLogics();
            
            // Load project plugins first
            std::string projectPath = project->getPath();
            if (!gameLogicManager->loadProjectPlugins(projectPath)) {
                spdlog::warn("PlayMode: Failed to load project plugins");
            }
            
            // Check if project specifies a game logic
            std::string gameLogicName = project->getGameLogic();
            if (!gameLogicName.empty()) {
                if (!gameLogicManager->createLogic(gameLogicName, playScene->registry)) {
                    spdlog::warn("PlayMode: Failed to create game logic '{}', continuing without it", gameLogicName);
                } else {
                    spdlog::info("PlayMode: Created game logic '{}'", gameLogicName);
                }
            }
        }
        
        state = PlayModeState::Playing;
        playTime = 0.0f;
        
        spdlog::info("PlayMode: Started playing scene");
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("PlayMode: Failed to start - {}", e.what());
        playScene.reset();
        return false;
    }
}

void PlayMode::stop() {
    if (state == PlayModeState::Stopped) {
        return;
    }
    
    // Clear game logics and unload plugins
    if (gameLogicManager) {
        gameLogicManager->clearLogics();
        gameLogicManager->unloadAllPlugins();
    }
    
    // Clean up play scene
    if (playScene) {
        playScene->onDestroy();
        playScene.reset();
    }
    
    state = PlayModeState::Stopped;
    playTime = 0.0f;
    editorScene = nullptr;
    gameLogicManager = nullptr;
    
    spdlog::info("PlayMode: Stopped");
}

void PlayMode::pause() {
    if (state == PlayModeState::Playing) {
        state = PlayModeState::Paused;
        spdlog::info("PlayMode: Paused");
    }
}

void PlayMode::resume() {
    if (state == PlayModeState::Paused) {
        state = PlayModeState::Playing;
        spdlog::info("PlayMode: Resumed");
    }
}

void PlayMode::update(float deltaTime, GameLogicManager* gameLogicManager) {
    if (state == PlayModeState::Playing && playScene) {
        playScene->onUpdate(deltaTime);
        
        // Update game logic for play scene with input state
        if (gameLogicManager) {
            InputState inputState = createInputState();
            
            
            gameLogicManager->update(playScene->registry, deltaTime, inputState);
        }
        
        playTime += deltaTime;
    }
}

void PlayMode::renderUI(Console* console) {
    if (!showPlayModeUI || state == PlayModeState::Stopped) {
        return;
    }
    
    // Draw play mode indicator
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Background bar
    int barHeight = 40;
    DrawRectangle(0, 0, screenWidth, barHeight, Fade(BLACK, 0.8f));
    
    // Status indicator
    const char* statusText = nullptr;
    Color statusColor = WHITE;
    
    switch (state) {
        case PlayModeState::Playing:
            statusText = "PLAYING";
            statusColor = GREEN;
            break;
        case PlayModeState::Paused:
            statusText = "PAUSED";
            statusColor = YELLOW;
            break;
        default:
            return;
    }
    
    // Draw status
    DrawText(statusText, 10, 10, 20, statusColor);
    
    // Draw play time
    std::stringstream timeStr;
    timeStr << "Time: " << std::fixed << std::setprecision(1) << playTime << "s";
    DrawText(timeStr.str().c_str(), 150, 10, 20, WHITE);
    
    // Draw controls hint
    const char* controls = state == PlayModeState::Playing ? 
        "Press F5 to stop, F6 to pause" : 
        "Press F5 to stop, F6 to resume";
    int controlsWidth = MeasureText(controls, 16);
    DrawText(controls, screenWidth - controlsWidth - 10, 12, 16, LIGHTGRAY);
    
    // Draw entity count if playing
    if (playScene) {
        auto view = playScene->registry.view<entt::entity>();
        int entityCount = 0;
        for (auto entity : view) {
            entityCount++;
        }
        
        std::string entityText = "Entities: " + std::to_string(entityCount);
        DrawText(entityText.c_str(), 300, 10, 20, WHITE);
    }
}

InputState PlayMode::createInputState() const {
    InputState inputState;
    
    // Common game keys
    std::vector<int> keysToCheck = {
        KEY_A, KEY_S, KEY_D, KEY_W,
        KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,
        KEY_SPACE, KEY_ENTER, KEY_ESCAPE,
        KEY_LEFT_SHIFT, KEY_LEFT_CONTROL, KEY_LEFT_ALT,
        KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE,
        KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE, KEY_ZERO
    };
    
    for (int key : keysToCheck) {
        inputState.keys[key] = IsKeyDown(key);
        inputState.keysPressed[key] = IsKeyPressed(key);
        inputState.keysReleased[key] = IsKeyReleased(key);
    }
    
    // Mouse position
    inputState.mouseX = GetMouseX();
    inputState.mouseY = GetMouseY();
    
    // Mouse buttons
    inputState.mouseButtons[MOUSE_LEFT_BUTTON] = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    inputState.mouseButtons[MOUSE_RIGHT_BUTTON] = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
    inputState.mouseButtons[MOUSE_MIDDLE_BUTTON] = IsMouseButtonDown(MOUSE_MIDDLE_BUTTON);
    
    return inputState;
}

} // namespace GameEngine