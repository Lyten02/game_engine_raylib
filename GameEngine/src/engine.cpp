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
#include <iomanip>
#include <sstream>

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
    
    // quit command override
    REGISTER_COMMAND(commandProcessor, "quit",
        [this](const std::vector<std::string>& args) {
            console->addLine("Shutting down...", YELLOW);
            requestQuit();
        }, "Quit the application");
}