#include "engine.h"
#include <raylib.h>
#include <spdlog/spdlog.h>
#include "systems/render_system.h"
#include "scene/scene.h"
#include "components/transform.h"
#include "components/sprite.h"
#include "resources/resource_manager.h"
#include "console/console.h"
#include "console/command_processor.h"
#include "console/command_registry.h"
#include "utils/config.h"
#include "scripting/script_manager.h"

// Standard library includes
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <filesystem>

// spdlog includes
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

Engine::Engine() = default;
Engine::~Engine() = default;

bool Engine::initialize() {
    spdlog::info("Engine::initialize - Starting engine initialization");
    
    // Load configuration
    if (!Config::load("config.json")) {
        spdlog::warn("Engine::initialize - Failed to load config.json, using defaults");
    }
    
    // Get window settings from config
    int width = Config::getInt("window.width", 1280);
    int height = Config::getInt("window.height", 720);
    std::string title = Config::getString("window.title", "Game Engine");
    bool fullscreen = Config::getBool("window.fullscreen", false);
    vsyncEnabled = Config::getBool("window.vsync", true);
    targetFPS = Config::getInt("window.target_fps", 60);
    
    // Configure window before initialization
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);  // High DPI support
    if (fullscreen) {
        SetConfigFlags(FLAG_FULLSCREEN_MODE);
    }
    
    // Initialize Raylib window
    InitWindow(width, height, title.c_str());
    
    if (!IsWindowReady()) {
        spdlog::error("Engine::initialize - Failed to create window");
        return false;
    }
    
    // Apply V-Sync and FPS settings from config
    if (vsyncEnabled) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
    SetTargetFPS(targetFPS);
    
    // Disable event waiting to prevent FPS drops
    SetExitKey(0);  // Disable ESC key exit
    
    // Initialize spdlog with file logging
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
    
    // Initialize resource manager
    resourceManager = std::make_unique<ResourceManager>();
    spdlog::info("Engine::initialize - Resource manager created");
    
    // Initialize ECS systems
    renderSystem = std::make_unique<RenderSystem>();
    renderSystem->initialize();
    spdlog::info("Engine::initialize - Render system created and initialized");
    
    // Initialize 2D camera
    Camera2D camera = {0};
    camera.target = {0.0f, 0.0f};
    camera.offset = {width / 2.0f, height / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    renderSystem->setCamera2D(camera);
    spdlog::info("Engine::initialize - 2D camera initialized");
    
    // Initialize console
    console = std::make_unique<Console>();
    console->initialize();
    
    // Initialize command processor
    commandProcessor = std::make_unique<CommandProcessor>();
    commandProcessor->initialize(console.get());
    console->setCommandProcessor(commandProcessor.get());
    
    // Register engine commands
    registerEngineCommands();
    
    spdlog::info("Engine::initialize - Console and command processor initialized");
    
    // Add initial console message
    console->addLine("Developer Console initialized. Press F1 to toggle.", YELLOW);
    console->addLine("Type 'help' for a list of commands.", GRAY);
    
    // Initialize script manager if enabled
    if (Config::getBool("scripting.lua_enabled", true)) {
        scriptManager = std::make_unique<ScriptManager>();
        if (scriptManager->initialize()) {
            spdlog::info("Engine::initialize - Script manager initialized");
            
            // Execute test script
            std::string scriptDir = Config::getString("scripting.script_directory", "scripts/");
            if (scriptManager->executeScript(scriptDir + "test.lua")) {
                console->addLine("Lua scripting initialized successfully", GREEN);
            }
        } else {
            spdlog::error("Engine::initialize - Failed to initialize script manager");
            scriptManager.reset();
        }
    }
    
    // Create a test scene
    currentScene = std::make_unique<Scene>();
    currentScene->onCreate();
    
    // Load test texture
    Texture2D* testTexture = resourceManager->loadTexture("assets/textures/test_sprite.png", "test_sprite");
    
    if (testTexture) {
        // Create a test entity with Transform and Sprite components
        auto testEntity = currentScene->registry.create();
        currentScene->registry.emplace<TransformComponent>(testEntity, 
            TransformComponent{
                Vector3{100.0f, 100.0f, 0.0f},    // Position
                Vector3{0.0f, 0.0f, 0.0f},        // No rotation
                Vector3{1.0f, 1.0f, 1.0f}         // Normal scale
            }
        );
        
        // Create sprite component with loaded texture
        Sprite& sprite = currentScene->registry.emplace<Sprite>(testEntity);
        sprite.texture = testTexture;
        sprite.sourceRect = {0.0f, 0.0f, 64.0f, 64.0f};  // Full texture
        sprite.tint = WHITE;
        
        spdlog::info("Engine::initialize - Test scene created with textured entity");
    } else {
        spdlog::error("Engine::initialize - Failed to load test texture");
    }
    
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
        totalTime += deltaTime;
        
        // Update console
        if (console) {
            console->update(deltaTime);
        }
        
        // Update the current scene
        if (currentScene && !console->isOpen()) {
            currentScene->onUpdate(deltaTime);
        }
        
        // Draw phase
        BeginDrawing();
        ClearBackground(GRAY);
        
        // Update render system with current scene's registry
        if (renderSystem && currentScene) {
            renderSystem->update(currentScene->registry);
        }
        
        // Render console on top
        if (console) {
            console->render();
        }
        
        // Draw help text if console is not open
        if (console && !console->isOpen()) {
            DrawText("Press F1 to open console", 10, 10, 20, LIGHTGRAY);
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
        
        EndDrawing();
    }
    
    spdlog::info("Engine::run - Main game loop ended");
}

void Engine::shutdown() {
    spdlog::info("Engine::shutdown - Shutting down engine");
    
    // Cleanup console
    if (console) {
        console->shutdown();
        console.reset();
        spdlog::info("Engine::shutdown - Console shut down");
    }
    
    // Cleanup command processor
    if (commandProcessor) {
        commandProcessor.reset();
        spdlog::info("Engine::shutdown - Command processor shut down");
    }
    
    // Cleanup script manager
    if (scriptManager) {
        scriptManager->shutdown();
        scriptManager.reset();
        spdlog::info("Engine::shutdown - Script manager shut down");
    }
    
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
    
    // Cleanup resources
    if (resourceManager) {
        resourceManager->unloadAll();
        resourceManager.reset();
        spdlog::info("Engine::shutdown - Resource manager cleaned up");
    }
    
    if (IsWindowReady()) {
        CloseWindow();
        spdlog::info("Engine::shutdown - Window closed");
    }
    
    running = false;
    spdlog::info("Engine::shutdown - Engine shutdown complete");
}

void Engine::registerEngineCommands() {
    // engine.info command
    REGISTER_COMMAND(commandProcessor, "engine.info", 
        [this](const std::vector<std::string>& args) {
            std::stringstream ss;
            ss << "Engine Information:\n";
            ss << "  FPS: " << GetFPS() << "\n";
            ss << "  Frame Time: " << std::fixed << std::setprecision(3) << GetFrameTime() * 1000 << " ms\n";
            ss << "  Total Time: " << std::fixed << std::setprecision(1) << totalTime << " s\n";
            ss << "  Window: " << GetScreenWidth() << "x" << GetScreenHeight();
            
            console->addLine(ss.str(), YELLOW);
        }, "Display engine information");
    
    // scene.info command
    REGISTER_COMMAND(commandProcessor, "scene.info",
        [this](const std::vector<std::string>& args) {
            if (!currentScene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            std::stringstream ss;
            ss << "Scene Information:\n";
            
            // Count entities
            int entityCount = 0;
            auto allEntities = currentScene->registry.view<entt::entity>();
            for (auto entity : allEntities) {
                entityCount++;
            }
            
            ss << "  Total Entities: " << entityCount;
            
            console->addLine(ss.str(), YELLOW);
        }, "Display current scene information");
    
    // entity.list command
    REGISTER_COMMAND(commandProcessor, "entity.list",
        [this](const std::vector<std::string>& args) {
            if (!currentScene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            console->addLine("Entity List:", YELLOW);
            auto view = currentScene->registry.view<entt::entity>();
            int count = 0;
            
            for (auto entity : view) {
                std::stringstream ss;
                ss << "  Entity " << static_cast<uint32_t>(entity) << ":";
                
                // Check for components
                if (currentScene->registry.all_of<TransformComponent>(entity)) {
                    ss << " [Transform]";
                }
                if (currentScene->registry.all_of<Sprite>(entity)) {
                    ss << " [Sprite]";
                }
                
                console->addLine(ss.str(), WHITE);
                count++;
                
                if (count > 20) {
                    console->addLine("  ... and more", GRAY);
                    break;
                }
            }
            
            console->addLine("Total: " + std::to_string(count) + " entities", GRAY);
        }, "List all entities and their components");
    
    // entity.create command
    REGISTER_COMMAND(commandProcessor, "entity.create",
        [this](const std::vector<std::string>& args) {
            if (!currentScene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            auto entity = currentScene->registry.create();
            
            // Add default transform
            currentScene->registry.emplace<TransformComponent>(entity,
                TransformComponent{
                    Vector3{200.0f + (rand() % 400), 200.0f + (rand() % 200), 0.0f},
                    Vector3{0.0f, 0.0f, 0.0f},
                    Vector3{1.0f, 1.0f, 1.0f}
                }
            );
            
            // If we have a test sprite, add it
            if (auto* texture = resourceManager->getTexture("test_sprite")) {
                Sprite& sprite = currentScene->registry.emplace<Sprite>(entity);
                sprite.texture = texture;
                Rectangle rect;
                rect.x = 0.0f;
                rect.y = 0.0f;
                rect.width = 64.0f;
                rect.height = 64.0f;
                sprite.sourceRect = rect;
                sprite.tint = WHITE;
            }
            
            console->addLine("Created entity " + std::to_string(static_cast<uint32_t>(entity)), GREEN);
        }, "Create a new test entity");
    
    // entity.destroy command
    REGISTER_COMMAND(commandProcessor, "entity.destroy",
        [this](const std::vector<std::string>& args) {
            if (!validateArgCount(console.get(), args, 1, "entity.destroy <id>")) {
                return;
            }
            
            if (!currentScene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            try {
                uint32_t id = std::stoul(args[0]);
                entt::entity entity = static_cast<entt::entity>(id);
                
                if (currentScene->registry.valid(entity)) {
                    currentScene->registry.destroy(entity);
                    console->addLine("Destroyed entity " + args[0], GREEN);
                } else {
                    console->addLine("Entity " + args[0] + " not found", RED);
                }
            } catch (const std::exception& e) {
                console->addLine("Invalid entity ID: " + args[0], RED);
            }
        }, "Destroy an entity by ID");
    
    // resource.list command
    REGISTER_COMMAND(commandProcessor, "resource.list",
        [this](const std::vector<std::string>& args) {
            console->addLine("Loaded Resources:", YELLOW);
            // Note: ResourceManager would need methods to list resources
            // For now, we know we have test_sprite
            console->addLine("  Texture: test_sprite", WHITE);
        }, "List all loaded resources");
    
    // render.stats command
    REGISTER_COMMAND(commandProcessor, "render.stats",
        ([this](const std::vector<std::string>& args) {
            if (!currentScene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            auto view = currentScene->registry.view<TransformComponent, Sprite>();
            size_t spriteCount = 0;
            for (auto entity : view) {
                const auto& sprite = view.get<Sprite>(entity);
                if (sprite.texture != nullptr) {
                    spriteCount++;
                }
            }
            
            std::stringstream ss;
            ss << "Render Statistics:\n";
            ss << "  FPS: " << GetFPS() << "\n";
            ss << "  Frame Time: " << std::fixed << std::setprecision(3) << GetFrameTime() * 1000 << " ms\n";
            ss << "  Sprites Rendered: " << spriteCount << "\n";
            ss << "  Draw Calls: ~" << spriteCount;
            
            console->addLine(ss.str(), YELLOW);
        }), "Display render system statistics");
    
    // quit command override
    REGISTER_COMMAND(commandProcessor, "quit",
        [this](const std::vector<std::string>& args) {
            console->addLine("Shutting down...", YELLOW);
            requestQuit();
        }, "Quit the application");
    
    // debug.toggle command
    REGISTER_COMMAND(commandProcessor, "debug.toggle",
        ([this](const std::vector<std::string>& args) {
            showDebugInfo = !showDebugInfo;
            console->addLine("Debug info " + std::string(showDebugInfo ? "enabled" : "disabled"), YELLOW);
        }), "Toggle debug info display");
    
    // console.fps command
    REGISTER_COMMAND(commandProcessor, "console.fps",
        ([this](const std::vector<std::string>& args) {
            bool currentState = console->isShowingFPS();
            console->setShowFPS(!currentState);
            console->addLine("Console FPS display " + std::string(!currentState ? "enabled" : "disabled"), YELLOW);
        }), "Toggle FPS display in console");
    
    // engine.fps command
    REGISTER_COMMAND(commandProcessor, "engine.fps",
        ([this](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: engine.fps <limit>", RED);
                console->addLine("  limit: 0 (unlimited), 30, 60, 120, 144, 240", GRAY);
                console->addLine("Current FPS limit: " + std::string(targetFPS == 0 ? "Unlimited" : std::to_string(targetFPS)), YELLOW);
                return;
            }
            
            try {
                int newFPS = std::stoi(args[0]);
                if (newFPS < 0) {
                    console->addLine("Invalid FPS limit. Use 0 for unlimited.", RED);
                    return;
                }
                
                targetFPS = newFPS;
                SetTargetFPS(targetFPS);
                
                if (targetFPS == 0) {
                    console->addLine("FPS limit removed - running at maximum speed", GREEN);
                } else {
                    console->addLine("FPS limit set to " + std::to_string(targetFPS), GREEN);
                }
            } catch (const std::exception& e) {
                console->addLine("Invalid FPS value: " + args[0], RED);
            }
        }), "Set FPS limit (0 for unlimited)");
    
    // engine.vsync command
    REGISTER_COMMAND(commandProcessor, "engine.vsync",
        ([this](const std::vector<std::string>& args) {
            vsyncEnabled = !vsyncEnabled;
            
            if (vsyncEnabled) {
                SetWindowState(FLAG_VSYNC_HINT);
                console->addLine("V-Sync enabled", GREEN);
            } else {
                ClearWindowState(FLAG_VSYNC_HINT);
                console->addLine("V-Sync disabled", GREEN);
            }
        }), "Toggle V-Sync");
    
    // engine.diag command
    REGISTER_COMMAND(commandProcessor, "engine.diag",
        ([this](const std::vector<std::string>& args) {
            std::stringstream ss;
            ss << "Performance Diagnostics:\n";
            ss << "  Window Focused: " << (IsWindowFocused() ? "Yes" : "No") << "\n";
            ss << "  Window Hidden: " << (IsWindowHidden() ? "Yes" : "No") << "\n";
            ss << "  Window Minimized: " << (IsWindowMinimized() ? "Yes" : "No") << "\n";
            ss << "  V-Sync: " << (vsyncEnabled ? "Enabled" : "Disabled") << "\n";
            ss << "  Target FPS: " << (targetFPS == 0 ? "Unlimited" : std::to_string(targetFPS)) << "\n";
            ss << "  Current FPS: " << GetFPS() << "\n";
            ss << "  Frame Time: " << std::fixed << std::setprecision(3) << GetFrameTime() * 1000 << " ms\n";
            
            #ifdef __APPLE__
            ss << "  Platform: macOS (pthread priority set)\n";
            #else
            ss << "  Platform: Other\n";
            #endif
            
            console->addLine(ss.str(), YELLOW);
        }), "Show performance diagnostics");
    
    // logs.open command
    REGISTER_COMMAND(commandProcessor, "logs.open",
        ([this](const std::vector<std::string>& args) {
            std::string logsPath = std::filesystem::absolute("logs").string();
            
            #ifdef _WIN32
                std::string command = "explorer \"" + logsPath + "\"";
            #elif __APPLE__
                std::string command = "open \"" + logsPath + "\"";
            #else
                std::string command = "xdg-open \"" + logsPath + "\"";
            #endif
            
            int result = std::system(command.c_str());
            if (result == 0) {
                console->addLine("Opened logs folder: " + logsPath, GREEN);
            } else {
                console->addLine("Failed to open logs folder", RED);
                console->addLine("Path: " + logsPath, GRAY);
            }
        }), "Open logs folder in file manager");
    
    // logs.list command
    REGISTER_COMMAND(commandProcessor, "logs.list",
        ([this](const std::vector<std::string>& args) {
            try {
                if (!std::filesystem::exists("logs")) {
                    console->addLine("No logs directory found", YELLOW);
                    return;
                }
                
                console->addLine("Log files:", YELLOW);
                int count = 0;
                
                for (const auto& entry : std::filesystem::directory_iterator("logs")) {
                    if (entry.path().extension() == ".log") {
                        auto fileSize = std::filesystem::file_size(entry.path());
                        std::stringstream ss;
                        ss << "  " << entry.path().filename().string() 
                           << " (" << fileSize / 1024 << " KB)";
                        console->addLine(ss.str(), WHITE);
                        count++;
                    }
                }
                
                if (count == 0) {
                    console->addLine("  No log files found", GRAY);
                } else {
                    console->addLine("Total: " + std::to_string(count) + " log files", GRAY);
                }
            } catch (const std::exception& e) {
                console->addLine("Error listing logs: " + std::string(e.what()), RED);
            }
        }), "List all log files");
    
    // config.reload command
    REGISTER_COMMAND(commandProcessor, "config.reload",
        ([this](const std::vector<std::string>& args) {
            Config::reload();
            console->addLine("Configuration reloaded", GREEN);
            
            // Apply new settings
            vsyncEnabled = Config::getBool("window.vsync", true);
            targetFPS = Config::getInt("window.target_fps", 60);
            
            if (vsyncEnabled) {
                SetWindowState(FLAG_VSYNC_HINT);
            } else {
                ClearWindowState(FLAG_VSYNC_HINT);
            }
            SetTargetFPS(targetFPS);
            
            console->addLine("Window settings updated", YELLOW);
        }), "Reload configuration from config.json");
    
    // config.get command
    REGISTER_COMMAND(commandProcessor, "config.get",
        ([this](const std::vector<std::string>& args) {
            if (args.empty()) {
                console->addLine("Usage: config.get <key>", RED);
                return;
            }
            
            auto value = Config::get(args[0]);
            if (value.is_null()) {
                console->addLine("Key not found: " + args[0], RED);
            } else {
                console->addLine(args[0] + " = " + value.dump(), YELLOW);
            }
        }), "Get configuration value by key");
    
    // config.set command
    REGISTER_COMMAND(commandProcessor, "config.set",
        ([this](const std::vector<std::string>& args) {
            if (args.size() < 2) {
                console->addLine("Usage: config.set <key> <value>", RED);
                return;
            }
            
            try {
                // Try to parse as JSON first
                nlohmann::json value = nlohmann::json::parse(args[1]);
                Config::set(args[0], value);
                console->addLine("Set " + args[0] + " = " + value.dump(), GREEN);
            } catch (...) {
                // If not valid JSON, treat as string
                Config::set(args[0], args[1]);
                console->addLine("Set " + args[0] + " = \"" + args[1] + "\"", GREEN);
            }
        }), "Set configuration value (runtime only)");
    
    // script.execute command
    if (scriptManager) {
        REGISTER_COMMAND(commandProcessor, "script.execute",
            ([this](const std::vector<std::string>& args) {
                if (args.empty()) {
                    console->addLine("Usage: script.execute <path>", RED);
                    return;
                }
                
                if (scriptManager->executeScript(args[0])) {
                    console->addLine("Script executed: " + args[0], GREEN);
                } else {
                    console->addLine("Failed to execute script: " + args[0], RED);
                }
            }), "Execute a Lua script");
        
        // script.reload command
        REGISTER_COMMAND(commandProcessor, "script.reload",
            ([this](const std::vector<std::string>& args) {
                if (args.empty()) {
                    console->addLine("Usage: script.reload <path>", RED);
                    return;
                }
                
                scriptManager->reloadScript(args[0]);
                console->addLine("Script reloaded: " + args[0], GREEN);
            }), "Reload and execute a Lua script");
        
        // script.list command
        REGISTER_COMMAND(commandProcessor, "script.list",
            ([this](const std::vector<std::string>& args) {
                auto scripts = scriptManager->getLoadedScripts();
                if (scripts.empty()) {
                    console->addLine("No scripts loaded", YELLOW);
                } else {
                    console->addLine("Loaded scripts:", YELLOW);
                    for (const auto& script : scripts) {
                        console->addLine("  " + script, WHITE);
                    }
                }
            }), "List all loaded scripts");
        
        // script.eval command
        REGISTER_COMMAND(commandProcessor, "script.eval",
            ([this](const std::vector<std::string>& args) {
                if (args.empty()) {
                    console->addLine("Usage: script.eval <lua code>", RED);
                    return;
                }
                
                // Join all arguments as Lua code
                std::string luaCode;
                for (size_t i = 0; i < args.size(); ++i) {
                    if (i > 0) luaCode += " ";
                    luaCode += args[i];
                }
                
                if (scriptManager->executeString(luaCode)) {
                    console->addLine("Lua code executed", GREEN);
                } else {
                    console->addLine("Lua execution failed", RED);
                }
            }), "Execute Lua code directly");
    }
}