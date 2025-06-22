#include "command_registry.h"
#include "../console/console.h"
#include "../console/command_processor.h"
#include "../scene/scene.h"
#include "../resources/resource_manager.h"
// Component headers removed - components are now optional
#include <entt/entt.hpp>
#include <raylib.h>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

namespace GameEngine {
void CommandRegistry::registerEntityCommands(CommandProcessor* processor, Console* console, std::function<Scene*()> getScene, ResourceManager* resourceManager) {
    // entity.list command
    processor->registerCommand("entity.list",
        [console, getScene](const std::vector<std::string>& args) {
            Scene* scene = getScene();
            if (!scene) {
                console->addLine("No active scene", RED);
                return;
            }

            auto& registry = scene->registry;
            int count = 0;

            console->addLine("Active entities:", YELLOW);
            auto view = registry.view<entt::entity>();
            for (auto entity : view) {
                std::stringstream ss;
                ss << "  Entity #" << (uint32_t)entity;

                // Component listing removed - components are now plugin-based
                // TODO: Add dynamic component enumeration via plugin API

                console->addLine(ss.str(), GRAY);
                count++;
            }

            console->addLine("Total entities: " + std::to_string(count), GREEN);
        }, "List all entities in the current scene", "Entity");

    // entity.create command {
        std::vector<CommandParameter> createParams = {
            {"x", "X position", false},
            {"y", "Y position", false},
            {"z", "Z position", false}
        };
        processor->registerCommand("entity.create",
        [console, getScene](const std::vector<std::string>& args) {
            Scene* scene = getScene();
            if (!scene) {
                console->addLine("No active scene", RED);
                return;
            }

            auto& registry = scene->registry;
            auto entity = registry.create();

            // No components are added automatically - plugins should add them
            // Position arguments are ignored since we don't have Transform component

            uint32_t entityId = (uint32_t)entity;
            console->addLine("Created entity #" + std::to_string(entityId) + " with Transform component", GREEN);

            // Set command data for CLI mode
            if (console->isCaptureMode()) {
                nlohmann::json data = {{"id", entityId}};
                console->setCommandData(data);
            }
        }, "Create a new entity", "Entity", "", createParams);
    }

    // entity.destroy command {
        std::vector<CommandParameter> destroyParams = {
            {"id", "Entity ID to destroy", true}
        };
        processor->registerCommand("entity.destroy",
        [console, getScene](const std::vector<std::string>& args) {
            Scene* scene = getScene();
            if (!scene) {
                console->addLine("No active scene", RED);
                return;
            }

            if (args.empty()) {
                console->addLine("Usage: entity.destroy <id>", RED);
                return;
            }

            auto& registry = scene->registry;
            uint32_t id = std::stoul(args[0]);
            auto entity = static_cast<entt::entity>(id);

            if (registry.valid(entity)) {
                registry.destroy(entity);
                console->addLine("Destroyed entity #" + args[0], GREEN);
            } else {
                console->addLine("Entity #" + args[0] + " not found", RED);
            }
        }, "Destroy an entity", "Entity", "", destroyParams);
    }
}

void CommandRegistry::registerResourceCommands(CommandProcessor* processor, Console* console) {
    // resource.list command
    processor->registerCommand("resource.list",
        [console](const std::vector<std::string>& args) {
            std::string type = args.empty() ? "all" : args[0];

            if (type == "all" || type == "textures") {
                // ResourceManager doesn't have getInstance() anymore, it's passed as parameter
                console->addLine("Resource listing not implemented yet", YELLOW);
                return;
                console->addLine("Loaded textures:", YELLOW);
                // TODO: Implement texture listing
            }

            if (type == "all" || type == "sounds") {
                // TODO: Implement sound listing
            }

            if (type == "all" || type == "music") {
                // TODO: Implement music listing
            }
        }, "List loaded resources", "Resource");
}

void CommandRegistry::registerRenderCommands(CommandProcessor* processor, Console* console, std::function<Scene*()> getScene) {
    // render.stats command
    processor->registerCommand("render.stats",
        [console, getScene](const std::vector<std::string>& args) {
            Scene* scene = getScene();
            if (!scene) {
                console->addLine("No active scene", RED);
                return;
            }

            // Render statistics are now plugin-specific
            console->addLine("Render statistics:", YELLOW);
            console->addLine("  Rendering is now handled by plugins", GRAY);
            console->addLine("  No built-in sprite components", GRAY);
            console->addLine("  FPS: " + std::to_string(GetFPS()), GREEN);
        }, "Display rendering statistics", "Render");
}

} // namespace GameEngine
