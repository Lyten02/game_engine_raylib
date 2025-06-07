#include "command_registry.h"
#include "../console/console.h"
#include "../console/command_processor.h"
#include "../scene/scene.h"
#include "../resources/resource_manager.h"
#include "../components/transform.h"
#include "../components/sprite.h"
#include <entt/entt.hpp>
#include <raylib.h>
#include <sstream>
#include <iomanip>

namespace GameEngine {

void CommandRegistry::registerEntityCommands(CommandProcessor* processor, Console* console, Scene** currentScene, ResourceManager* resourceManager) {
    // entity.list command
    processor->registerCommand("entity.list",
        [console, currentScene](const std::vector<std::string>& args) {
            if (!*currentScene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            auto& registry = (*currentScene)->registry;
            int count = 0;
            
            console->addLine("Active entities:", YELLOW);
            auto view = registry.view<entt::entity>();
            for (auto entity : view) {
                std::stringstream ss;
                ss << "  Entity #" << (uint32_t)entity;
                
                // Check what components it has
                bool hasTransform = registry.all_of<TransformComponent>(entity);
                bool hasSprite = registry.all_of<Sprite>(entity);
                
                if (hasTransform || hasSprite) {
                    ss << " [";
                    if (hasTransform) ss << "Transform";
                    if (hasTransform && hasSprite) ss << ", ";
                    if (hasSprite) ss << "Sprite";
                    ss << "]";
                }
                
                console->addLine(ss.str(), GRAY);
                count++;
            }
            
            console->addLine("Total entities: " + std::to_string(count), GREEN);
        }, "List all entities in the current scene", "Entity");
    
    // entity.create command
    {
        std::vector<CommandParameter> createParams = {
            {"x", "X position", false},
            {"y", "Y position", false},
            {"z", "Z position", false}
        };
        processor->registerCommand("entity.create",
        [console, currentScene](const std::vector<std::string>& args) {
            if (!*currentScene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            auto& registry = (*currentScene)->registry;
            auto entity = registry.create();
            
            // Add transform component
            auto& transform = registry.emplace<TransformComponent>(entity);
            
            // Parse position if provided
            try {
                if (args.size() >= 1 && !args[0].empty()) transform.position.x = std::stof(args[0]);
                if (args.size() >= 2 && !args[1].empty()) transform.position.y = std::stof(args[1]);
                if (args.size() >= 3 && !args[2].empty()) transform.position.z = std::stof(args[2]);
            } catch (const std::exception& e) {
                // Ignore parsing errors, use default position
            }
            
            console->addLine("Created entity #" + std::to_string((uint32_t)entity) + " with Transform component", GREEN);
        }, "Create a new entity", "Entity", "", createParams);
    }
    
    // entity.destroy command
    {
        std::vector<CommandParameter> destroyParams = {
            {"id", "Entity ID to destroy", true}
        };
        processor->registerCommand("entity.destroy",
        [console, currentScene](const std::vector<std::string>& args) {
            if (!*currentScene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            if (args.empty()) {
                console->addLine("Usage: entity.destroy <id>", RED);
                return;
            }
            
            auto& registry = (*currentScene)->registry;
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

void CommandRegistry::registerRenderCommands(CommandProcessor* processor, Console* console, Scene** currentScene) {
    // render.stats command
    processor->registerCommand("render.stats",
        [console, currentScene](const std::vector<std::string>& args) {
            if (!*currentScene) {
                console->addLine("No active scene", RED);
                return;
            }
            
            auto& registry = (*currentScene)->registry;
            auto view = registry.view<TransformComponent, Sprite>();
            
            int spriteCount = 0;
            int visibleCount = 0;
            
            for (auto entity : view) {
                spriteCount++;
                auto& sprite = view.get<Sprite>(entity);
                // All sprites are visible by default
                visibleCount++;
            }
            
            std::stringstream ss;
            ss << "Render Statistics:\n";
            ss << "  Total sprites: " << spriteCount << "\n";
            ss << "  Visible sprites: " << visibleCount << "\n";
            ss << "  FPS: " << GetFPS();
            
            console->addLine(ss.str(), YELLOW);
        }, "Display rendering statistics", "Render");
}

} // namespace GameEngine