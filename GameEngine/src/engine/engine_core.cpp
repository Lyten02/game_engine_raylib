#include "engine_core.h"
#include <raylib.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "../utils/config.h"
#include "../resources/resource_manager.h"
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <vector>

namespace GameEngine {

EngineCore::EngineCore() = default;
EngineCore::~EngineCore() = default;

bool EngineCore::initialize(bool headless) {
    headlessMode = headless;
    
    if (!headlessMode) {
        spdlog::info("EngineCore::initialize - Starting engine initialization");
    } else {
        Config::setSilentMode(true);
        spdlog::set_level(spdlog::level::off);
    }
    
    // Load configuration
    if (!Config::load("config.json")) {
        if (!headlessMode) {
            spdlog::warn("EngineCore::initialize - Failed to load config.json, using defaults");
        }
    }
    
    if (headlessMode) {
        return initializeHeadless();
    } else {
        return initializeGraphics();
    }
}

bool EngineCore::initializeGraphics() {
    // Get window settings from config
    int width = Config::getInt("window.width", 1280);
    int height = Config::getInt("window.height", 720);
    std::string title = Config::getString("window.title", "Game Engine");
    bool fullscreen = Config::getBool("window.fullscreen", false);
    vsyncEnabled = Config::getBool("window.vsync", true);
    targetFPS = Config::getInt("window.target_fps", 60);
    
    // Configure window before initialization
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    if (fullscreen) {
        SetConfigFlags(FLAG_FULLSCREEN_MODE);
    }
    
    // Initialize Raylib window
    InitWindow(width, height, title.c_str());
    
    if (!IsWindowReady()) {
        spdlog::error("EngineCore::initialize - Failed to create window");
        return false;
    }
    
    // Apply V-Sync and FPS settings
    if (vsyncEnabled) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
    SetTargetFPS(targetFPS);
    
    // Disable event waiting to prevent FPS drops
    SetExitKey(0);  // Disable ESC key exit
    
    // Initialize logging
    initializeLogging();
    
    running = true;
    spdlog::info("EngineCore::initialize - Engine core initialized successfully ({}x{}, \"{}\")", 
                 width, height, title);
    
    return true;
}

bool EngineCore::initializeHeadless() {
    // In headless mode, set up minimal logging
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>("engine", console_sink);
        
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::err);
        spdlog::flush_on(spdlog::level::err);
    } catch (const std::exception& e) {
        // Silent failure - we're in headless mode
    }
    
    running = true;
    return true;
}

void EngineCore::initializeLogging() {
    try {
        // Create logs directory
        std::filesystem::create_directories("logs");
        
        // Create file sink with timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream logFileName;
        logFileName << "logs/engine_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".log";
        
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFileName.str());
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("engine", sinks.begin(), sinks.end());
        
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::info);
        spdlog::flush_on(spdlog::level::info);
        
        spdlog::info("Log file created: {}", logFileName.str());
    } catch (const std::exception& e) {
        spdlog::error("Failed to create log file: {}", e.what());
        spdlog::set_level(spdlog::level::info);
    }
}

void EngineCore::processFrame(float deltaTime) {
    totalTime += deltaTime;
}

bool EngineCore::shouldContinueRunning() const {
    return running && !WindowShouldClose();
}

void EngineCore::beginFrame() {
    BeginDrawing();
}

void EngineCore::endFrame() {
    EndDrawing();
}

void EngineCore::clearBackground() {
    ClearBackground(GRAY);
}

void EngineCore::shutdown() {
    spdlog::info("EngineCore::shutdown - Shutting down engine core");
    
    // Clean up default texture before closing window
    cleanupDefaultTexture();
    
    if (!headlessMode && IsWindowReady()) {
        CloseWindow();
        spdlog::info("EngineCore::shutdown - Window closed");
    }
    
    running = false;
    spdlog::info("EngineCore::shutdown - Engine core shutdown complete");
}

bool EngineCore::isWindowReady() const {
    return !headlessMode && IsWindowReady();
}

int EngineCore::getScreenWidth() const {
    return GetScreenWidth();
}

int EngineCore::getScreenHeight() const {
    return GetScreenHeight();
}

int EngineCore::getFPS() const {
    return GetFPS();
}

float EngineCore::getFrameTime() const {
    return GetFrameTime();
}

void EngineCore::setTargetFPS(int fps) {
    targetFPS = fps;
    SetTargetFPS(targetFPS);
}

void EngineCore::setVSync(bool enabled) {
    vsyncEnabled = enabled;
    if (vsyncEnabled) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
}

} // namespace GameEngine