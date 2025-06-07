#pragma once

#include <memory>
#include <string>
#include "../scene/scene.h"

class Console;

namespace GameEngine {

class Project;

enum class PlayModeState {
    Stopped,
    Playing,
    Paused
};

class PlayMode {
private:
    PlayModeState state = PlayModeState::Stopped;
    std::unique_ptr<Scene> playScene;
    Scene* editorScene = nullptr;
    bool showPlayModeUI = true;
    float playTime = 0.0f;
    
public:
    PlayMode() = default;
    ~PlayMode() = default;
    
    // Start playing the current scene
    bool start(Scene* currentScene, Project* project);
    
    // Stop playing and restore editor scene
    void stop();
    
    // Pause/Resume
    void pause();
    void resume();
    
    // Update play mode
    void update(float deltaTime);
    
    // Render play mode UI
    void renderUI(Console* console);
    
    // State getters
    bool isPlaying() const { return state == PlayModeState::Playing; }
    bool isPaused() const { return state == PlayModeState::Paused; }
    bool isStopped() const { return state == PlayModeState::Stopped; }
    PlayModeState getState() const { return state; }
    
    Scene* getPlayScene() { return playScene.get(); }
    float getPlayTime() const { return playTime; }
    
    void setShowUI(bool show) { showPlayModeUI = show; }
    bool isShowingUI() const { return showPlayModeUI; }
};

} // namespace GameEngine