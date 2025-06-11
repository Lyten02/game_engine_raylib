#pragma once

#include <memory>
#include <string>

// Forward declarations
class Scene;
struct Camera2D;

namespace GameEngine {

class EngineCore {
public:
    EngineCore();
    ~EngineCore();

    // Initialization methods
    bool initialize(bool headlessMode);
    bool initializeGraphics();
    bool initializeHeadless();
    
    // Main game loop control
    void processFrame(float deltaTime);
    bool shouldContinueRunning() const;
    
    // Shutdown
    void shutdown();
    
    // Window and render control
    void beginFrame();
    void endFrame();
    void clearBackground();
    
    // State getters
    bool isRunning() const { return running; }
    bool isHeadless() const { return headlessMode; }
    bool isWindowReady() const;
    float getTotalTime() const { return totalTime; }
    
    // Window properties
    int getScreenWidth() const;
    int getScreenHeight() const;
    int getFPS() const;
    float getFrameTime() const;
    
    // Settings
    void setTargetFPS(int fps);
    void setVSync(bool enabled);
    bool isVSyncEnabled() const { return vsyncEnabled; }
    int getTargetFPS() const { return targetFPS; }
    
    // Engine control
    void requestQuit() { running = false; }

private:
    // Core state
    bool running = false;
    bool headlessMode = false;
    float totalTime = 0.0f;
    
    // Window settings
    int targetFPS = 60;
    bool vsyncEnabled = true;
    
    // Initialize logging
    void initializeLogging();
    void displayEnginePaths();
};

} // namespace GameEngine