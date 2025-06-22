#include "command_registry.h"
#include "engine_core.h"
#include "../engine.h"
#include "../console/console.h"
#include "../console/command_processor.h"
#include "../scene/scene.h"
#include "../resources/resource_manager.h"
#include "../scripting/script_manager.h"
#include "../project/project_manager.h"
#include "../build/build_system.h"
#include "../build/async_build_system.h"
#include "../engine/play_mode.h"
#include "../utils/config.h"
#include "../utils/engine_paths.h"
#include <filesystem>

namespace GameEngine {
CommandRegistry::CommandRegistry() = default;
CommandRegistry::~CommandRegistry() = default;

void CommandRegistry::registerAllCommands(CommandProcessor* processor,
                                        EngineCore* engineCore,
                                        Console* console,
                                        std::function<Scene*()> getScene,
                                        ResourceManager* resourceManager,
                                        ScriptManager* scriptManager,
                                        GameLogicManager* gameLogicManager,
                                        ProjectManager* projectManager,
                                        BuildSystem* buildSystem,
                                        AsyncBuildSystem* asyncBuildSystem,
                                        PlayMode* playMode,
                                        PackageManager* packageManager,
                                        Engine* engine) {
    // Store the showDebugInfo pointer
    static bool debugInfo = true;
    showDebugInfo = &debugInfo;

    // Register commands by category
    registerEngineCommands(processor, engineCore, console);
    registerSceneCommands(processor, console, getScene, projectManager);
    registerEntityCommands(processor, console, getScene, resourceManager);
    registerResourceCommands(processor, console);
    registerRenderCommands(processor, console, getScene);
    registerDebugCommands(processor, console, showDebugInfo);
    registerConsoleCommands(processor, console);
    registerConfigCommands(processor, console, engineCore);

    if (scriptManager) {
        registerScriptCommands(processor, console, scriptManager);
    }

    if (gameLogicManager) {
        registerGameLogicCommands(processor, console, gameLogicManager, getScene);
    }

    registerProjectCommands(processor, console, projectManager, getScene, engine);
    registerBuildCommands(processor, console, projectManager, buildSystem, asyncBuildSystem);
    registerPlayModeCommands(processor, console, getScene, projectManager, playMode);
    registerLogCommands(processor, console);

    if (packageManager) {
        registerPackageCommands(processor, console, packageManager);
    }
}

// Helper methods
std::vector<std::string> CommandRegistry::getSceneList(ProjectManager* projectManager) const {
    std::vector<std::string> scenes;

    if (projectManager && projectManager->getCurrentProject()) {
        std::string scenesPath = projectManager->getCurrentProject()->getPath() + "/scenes";

        if (std::filesystem::exists(scenesPath)) {
            for (const auto& entry : std::filesystem::directory_iterator(scenesPath)) {
                if (entry.path().extension() == ".json") {
                    scenes.push_back(entry.path().stem().string());
                }
            }
        }
    }

    return scenes;
}

std::vector<std::string> CommandRegistry::getProjectList() const {
    std::vector<std::string> projects;

    std::filesystem::path projectsDir = EnginePaths::getProjectsDir();
    if (std::filesystem::exists(projectsDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(projectsDir)) {
            if (entry.is_directory()) {
                std::filesystem::path projectFile = entry.path() / "project.json";
                if (std::filesystem::exists(projectFile)) {
                    projects.push_back(entry.path().filename().string());
                }
            }
        }
    }

    return projects;
}

std::vector<std::string> CommandRegistry::getScriptList() const {
    std::vector<std::string> scripts;

    std::string scriptDir = Config::getString("scripting.script_directory", "scripts/");
    if (std::filesystem::exists(scriptDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(scriptDir)) {
            if (entry.path().extension() == ".lua") {
                scripts.push_back(entry.path().filename().string());
            }
        }
    }

    return scripts;
}

std::vector<std::string> CommandRegistry::getConfigKeys() const {
    std::vector<std::string> keys;

    keys.push_back("window.width");
    keys.push_back("window.height");
    keys.push_back("window.title");
    keys.push_back("window.fullscreen");
    keys.push_back("window.vsync");
    keys.push_back("window.target_fps");
    keys.push_back("console.font_size");
    keys.push_back("console.max_lines");
    keys.push_back("console.background_alpha");
    keys.push_back("scripting.lua_enabled");
    keys.push_back("scripting.script_directory");
    keys.push_back("graphics.antialiasing");
    keys.push_back("graphics.texture_filter");

    return keys;
}

} // namespace GameEngine
