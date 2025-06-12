#include "systems_manager.h"
#include "../systems/render_system.h"
#include "../resources/resource_manager.h"
#include "../console/console.h"
#include "../console/command_processor.h"
#include "../scripting/script_manager.h"
#include "../scripting/game_logic_manager.h"
#include "../project/project_manager.h"
#include "../build/build_system.h"
#include "../build/async_build_system.h"
#include "../engine/play_mode.h"
#include "../serialization/component_registry.h"
#include "../components/transform.h"
#include "../components/sprite.h"
#include "../utils/config.h"
#include <raylib.h>
#include <spdlog/spdlog.h>

namespace GameEngine {

SystemsManager::SystemsManager() = default;
SystemsManager::~SystemsManager() = default;

bool SystemsManager::initialize(bool headless) {
    headlessMode = headless;
    
    // Initialize resource manager first (needed by other systems)
    if (!initializeResourceManager(headlessMode)) {
        return false;
    }
    
    // Initialize render system (only in graphics mode)
    if (!headlessMode && !initializeRenderSystem(headlessMode)) {
        return false;
    }
    
    // Initialize console and command processor
    if (!initializeConsole()) {
        return false;
    }
    
    // Initialize optional systems
    initializeScriptManager();
    initializeGameLogicManager();
    initializeProjectManager();
    initializeBuildSystems();
    
    if (!headlessMode) {
        initializePlayMode();
    }
    
    // Register components for serialization
    registerComponents();
    
    spdlog::info("SystemsManager::initialize - All systems initialized successfully");
    return true;
}

bool SystemsManager::initializeResourceManager(bool headlessMode) {
    resourceManager = std::make_unique<ResourceManager>();
    resourceManager->setHeadlessMode(headlessMode);
    resourceManager->setRayLibInitialized(!headlessMode);
    
    if (!headlessMode) {
        resourceManager->setSilentMode(false);
    } else {
        resourceManager->setSilentMode(true);
    }
    
    spdlog::info("SystemsManager::initialize - Resource manager created");
    return true;
}

bool SystemsManager::initializeRenderSystem(bool headlessMode) {
    if (headlessMode) {
        return true; // Skip in headless mode
    }
    
    renderSystem = std::make_unique<RenderSystem>();
    renderSystem->initialize();
    
    // Initialize 2D camera
    int width = GetScreenWidth();
    int height = GetScreenHeight();
    
    Camera2D camera = {0};
    camera.target = {0.0f, 0.0f};
    camera.offset = {width / 2.0f, height / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    renderSystem->setCamera2D(camera);
    
    spdlog::info("SystemsManager::initialize - Render system created and initialized");
    return true;
}

bool SystemsManager::initializeConsole() {
    console = std::make_unique<Console>();
    console->initialize();
    
    commandProcessor = std::make_unique<CommandProcessor>();
    commandProcessor->initialize(console.get());
    console->setCommandProcessor(commandProcessor.get());
    
    spdlog::info("SystemsManager::initialize - Console and command processor initialized");
    
    if (!headlessMode) {
        console->addLine("Developer Console initialized. Press F1 to toggle.", YELLOW);
        console->addLine("Type 'help' for a list of commands.", GRAY);
    }
    
    return true;
}

bool SystemsManager::initializeScriptManager() {
    if (!Config::getBool("scripting.lua_enabled", true)) {
        return true; // Scripting disabled
    }
    
    scriptManager = std::make_unique<ScriptManager>();
    if (scriptManager->initialize()) {
        spdlog::info("SystemsManager::initialize - Script manager initialized");
        
        if (!headlessMode) {
            // Execute test script
            std::string scriptDir = Config::getString("scripting.script_directory", "scripts/");
            if (scriptManager->executeScript(scriptDir + "test.lua")) {
                console->addLine("Lua scripting initialized successfully", GREEN);
            }
        }
        
        return true;
    } else {
        spdlog::error("SystemsManager::initialize - Failed to initialize script manager");
        scriptManager.reset();
        return false;
    }
}

bool SystemsManager::initializeProjectManager() {
    projectManager = std::make_unique<ProjectManager>();
    spdlog::info("SystemsManager::initialize - Project manager initialized");
    
    if (!headlessMode) {
        console->addLine("Project Manager initialized. Use 'project.create' or 'project.open' to begin.", YELLOW);
    }
    
    return true;
}

bool SystemsManager::initializeBuildSystems() {
    buildSystem = std::make_unique<BuildSystem>();
    asyncBuildSystem = std::make_unique<AsyncBuildSystem>();
    spdlog::info("SystemsManager::initialize - Build systems initialized");
    return true;
}

bool SystemsManager::initializePlayMode() {
    playMode = std::make_unique<PlayMode>();
    spdlog::info("SystemsManager::initialize - Play mode initialized");
    return true;
}

bool SystemsManager::initializeGameLogicManager() {
    gameLogicManager = std::make_unique<GameLogicManager>();
    if (gameLogicManager->initialize()) {
        spdlog::info("SystemsManager::initialize - Game logic manager initialized");
        
        if (!headlessMode) {
            console->addLine("Game Logic Manager initialized. C++ game logic system ready.", GREEN);
        }
        
        return true;
    } else {
        spdlog::error("SystemsManager::initialize - Failed to initialize game logic manager");
        gameLogicManager.reset();
        return false;
    }
}

void SystemsManager::registerComponents() {
    ComponentRegistry::getInstance().registerComponent<TransformComponent>("Transform");
    ComponentRegistry::getInstance().registerComponent<Sprite>("Sprite");
    spdlog::info("SystemsManager::initialize - Components registered for serialization");
}

void SystemsManager::shutdown() {
    spdlog::info("SystemsManager::shutdown - Shutting down all systems");
    
    // Shutdown in reverse order of initialization
    if (playMode) {
        playMode.reset();
        spdlog::info("SystemsManager::shutdown - Play mode shut down");
    }
    
    if (asyncBuildSystem) {
        asyncBuildSystem.reset();
    }
    
    if (buildSystem) {
        buildSystem.reset();
        spdlog::info("SystemsManager::shutdown - Build systems shut down");
    }
    
    if (projectManager) {
        projectManager->closeProject();
        projectManager.reset();
        spdlog::info("SystemsManager::shutdown - Project manager shut down");
    }
    
    if (scriptManager) {
        scriptManager->shutdown();
        scriptManager.reset();
        spdlog::info("SystemsManager::shutdown - Script manager shut down");
    }
    
    if (gameLogicManager) {
        gameLogicManager->shutdown();
        gameLogicManager.reset();
        spdlog::info("SystemsManager::shutdown - Game logic manager shut down");
    }
    
    if (console) {
        console->shutdown();
        console.reset();
        spdlog::info("SystemsManager::shutdown - Console shut down");
    }
    
    if (commandProcessor) {
        commandProcessor.reset();
        spdlog::info("SystemsManager::shutdown - Command processor shut down");
    }
    
    if (renderSystem) {
        renderSystem->shutdown();
        renderSystem.reset();
        spdlog::info("SystemsManager::shutdown - Render system shut down");
    }
    
    if (resourceManager) {
        resourceManager->unloadAll();
        resourceManager.reset();
        spdlog::info("SystemsManager::shutdown - Resource manager cleaned up");
    }
    
    spdlog::info("SystemsManager::shutdown - All systems shut down complete");
}

} // namespace GameEngine