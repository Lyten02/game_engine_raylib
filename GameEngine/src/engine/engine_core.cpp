#include "engine_core.h"
#include <raylib.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "../utils/config.h"
#include "../utils/engine_paths.h"
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

    // Initialize EnginePaths first
    EnginePaths::initialize();

    if (!headlessMode) {
        spdlog::info("EngineCore::initialize - Starting engine initialization");
    } else {
        Config::setSilentMode(true);
        spdlog::set_level(spdlog::level::off);
    }

    // Load configuration using EnginePaths
    std::string configPath = EnginePaths::getConfigFile().string();
    if (!Config::load(configPath)) {
        if (!headlessMode) {
            spdlog::warn("EngineCore::initialize - Failed to load {}, using defaults", configPath);
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

    // Display engine paths information
    displayEnginePaths();

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
        // Create logs directory using EnginePaths
        std::filesystem::path logsDir = EnginePaths::getLogsDir();
        std::filesystem::create_directories(logsDir);

        // Create file sink with timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream logFileName;
        logFileName << "engine_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".log";

        std::filesystem::path logFilePath = logsDir / logFileName.str();

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string());
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("engine", sinks.begin(), sinks.end());

        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::info);
        spdlog::flush_on(spdlog::level::info);

        spdlog::info("Log file created: {}", logFilePath.string());
    } catch (const std::exception& e) {
        spdlog::error("Failed to create log file: {}", e.what());
        spdlog::set_level(spdlog::level::info);
    }
}

void EngineCore::processFrame(float deltaTime) {
    totalTime += deltaTime;
}

bool EngineCore::shouldContinueRunning() const {
    if (headlessMode) {
        return running; // In headless mode, only check the running flag
    }
    return running && !WindowShouldClose();
}

void EngineCore::beginFrame() {
    if (!headlessMode) {
        BeginDrawing();
    }
}

void EngineCore::endFrame() {
    if (!headlessMode) {
        EndDrawing();
    }
}

void EngineCore::clearBackground() {
    if (!headlessMode) {
        ClearBackground(GRAY);
    }
}

void EngineCore::shutdown() {
    spdlog::info("EngineCore::shutdown - Shutting down engine core");

    // Default texture cleanup is now handled automatically by ResourceManager destructor

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
    if (headlessMode) {
        return 1280; // Virtual screen width for headless mode
    }
    return GetScreenWidth();
}

int EngineCore::getScreenHeight() const {
    if (headlessMode) {
        return 720; // Virtual screen height for headless mode
    }
    return GetScreenHeight();
}

int EngineCore::getFPS() const {
    if (headlessMode) {
        return targetFPS; // Return target FPS in headless mode
    }
    return GetFPS();
}

float EngineCore::getFrameTime() const {
    if (headlessMode) {
        return 1.0f / targetFPS; // Fixed frame time in headless mode
    }
    return GetFrameTime();
}

void EngineCore::setTargetFPS(int fps) {
    targetFPS = fps;
    if (!headlessMode) {
        SetTargetFPS(targetFPS);
    }
}

void EngineCore::setVSync(bool enabled) {
    vsyncEnabled = enabled;
    if (!headlessMode) {
        if (vsyncEnabled) {
            SetWindowState(FLAG_VSYNC_HINT);
        } else {
            ClearWindowState(FLAG_VSYNC_HINT);
        }
    }
}

void EngineCore::displayEnginePaths() {
    // Use centralized EnginePaths for display
    EnginePaths::displayPaths();
}

} // namespace GameEngine
