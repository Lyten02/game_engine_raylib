#include "play_mode.h"
#include "../scene/scene.h"
#include "../console/console.h"
#include "../project/project.h"
#include "../serialization/scene_serializer.h"
#include "../components/transform.h"
#include "../components/sprite.h"
#include <raylib.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>

namespace GameEngine {

bool PlayMode::start(Scene* currentScene, Project* project) {
    if (state != PlayModeState::Stopped || !currentScene) {
        return false;
    }
    
    try {
        // Store reference to editor scene
        editorScene = currentScene;
        
        // Create play scene as a copy of the current scene
        playScene = std::make_unique<Scene>();
        playScene->onCreate();
        
        // Serialize current scene to JSON
        nlohmann::json sceneData = SceneSerializer::sceneToJson(currentScene);
        
        // Deserialize into play scene
        SceneSerializer::jsonToScene(sceneData, playScene.get());
        
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
    
    // Clean up play scene
    if (playScene) {
        playScene->onDestroy();
        playScene.reset();
    }
    
    state = PlayModeState::Stopped;
    playTime = 0.0f;
    editorScene = nullptr;
    
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

void PlayMode::update(float deltaTime) {
    if (state == PlayModeState::Playing && playScene) {
        playScene->onUpdate(deltaTime);
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

} // namespace GameEngine