#include "package_loader.h"
#include "../plugins/plugin_manager.h"
#include <spdlog/spdlog.h>

// Include package components and systems
#include "../../packages/physics-2d/components/rigid_body.h"
#include "../../packages/physics-2d/components/box_collider.h"
#include "../../packages/physics-2d/systems/physics_system.h"

namespace GameEngine {
PackageLoader::PackageLoader() {
    registerBuiltinComponents();
    registerBuiltinSystems();
}

PackageLoader::~PackageLoader() = default;

bool PackageLoader::loadPackageResources(const Package& package, const std::filesystem::path& packagePath) {
    spdlog::info("[PackageLoader] Loading resources for package: {}", package.getName());

    // Check if package has plugin
    if (package.hasPluginInfo()) {
        if (!pluginManager) {
            spdlog::error("[PackageLoader] Plugin manager not set, cannot load plugins");
            lastError = "Plugin manager not available";
            return false;
        }

        const auto& pluginInfo = package.getPluginInfo();
        auto pluginPath = packagePath / pluginInfo.library;

        if (!pluginManager->loadPlugin(pluginPath.string())) {
            spdlog::error("[PackageLoader] Failed to load plugin: {}", pluginPath.string());
            lastError = "Failed to load plugin: " + pluginPath.string();
            return false;
        }

        spdlog::info("[PackageLoader] Successfully loaded plugin: {}", pluginPath.string());
    }

    // Load components (both from plugins and built-in)
    for (const auto& component : package.getComponents()) {
        if (!loadComponentFromFile(component, packagePath)) {
            spdlog::error("[PackageLoader] Failed to load component: {}", component.name);
            return false;
        }
    }

    // Load systems (both from plugins and built-in)
    for (const auto& system : package.getSystems()) {
        if (!loadSystemFromFile(system, packagePath)) {
            spdlog::error("[PackageLoader] Failed to load system: {}", system.name);
            return false;
        }
    }

    spdlog::info("[PackageLoader] Successfully loaded package resources for: {}", package.getName());
    return true;
}

void PackageLoader::registerComponent(const std::string& name, ComponentFactory factory) {
    if (componentFactories.find(name) != componentFactories.end()) {
        spdlog::warn("[PackageLoader] Component already registered: {}", name);
        return;
    }

    componentFactories[name] = factory;
    spdlog::debug("[PackageLoader] Registered component: {}", name);
}

bool PackageLoader::hasComponent(const std::string& name) const {
    return componentFactories.find(name) != componentFactories.end();
}

ComponentFactory PackageLoader::getComponentFactory(const std::string& name) const {
    auto it = componentFactories.find(name);
    if (it != componentFactories.end()) {
        return it->second;
    }
    return nullptr;
}

void PackageLoader::registerSystem(const std::string& name, SystemFactory factory) {
    if (systemFactories.find(name) != systemFactories.end()) {
        spdlog::warn("[PackageLoader] System already registered: {}", name);
        return;
    }

    systemFactories[name] = factory;
    spdlog::debug("[PackageLoader] Registered system: {}", name);
}

bool PackageLoader::hasSystem(const std::string& name) const {
    return systemFactories.find(name) != systemFactories.end();
}

SystemFactory PackageLoader::getSystemFactory(const std::string& name) const {
    auto it = systemFactories.find(name);
    if (it != systemFactories.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> PackageLoader::getRegisteredComponents() const {
    std::vector<std::string> result;
    for (const auto& [name, factory] : componentFactories) {
        result.push_back(name);
    }
    return result;
}

std::vector<std::string> PackageLoader::getRegisteredSystems() const {
    std::vector<std::string> result;
    for (const auto& [name, factory] : systemFactories) {
        result.push_back(name);
    }
    return result;
}

void PackageLoader::registerBuiltinComponents() {
    // Register physics components
    registerComponent("RigidBody", [](entt::registry& registry, entt::entity entity) {
        registry.emplace<Physics::RigidBody>(entity);
    });

    registerComponent("BoxCollider", [](entt::registry& registry, entt::entity entity) {
        registry.emplace<Physics::BoxCollider>(entity);
    });

    // Register other built-in components as they are implemented
}

// Adapter to make PhysicsSystem work with ISystem interface
class PhysicsSystemAdapter : public ISystem {
private:
    Physics::PhysicsSystem physicsSystem;

public:
    void initialize() override {
        physicsSystem.initialize();
    }

    void update(entt::registry& registry, float deltaTime) override {
        physicsSystem.update(registry, deltaTime);
    }

    void shutdown() override {
        physicsSystem.shutdown();
    }
};

void PackageLoader::registerBuiltinSystems() {
    // Register physics system
    registerSystem("PhysicsSystem", []() -> std::unique_ptr<ISystem> {
        return std::make_unique<PhysicsSystemAdapter>();
    });

    // Register other built-in systems as they are implemented
}

bool PackageLoader::loadComponentFromFile(const ComponentInfo& component, const std::filesystem::path& basePath) {
    // Check built-in registry first
    if (hasComponent(component.name)) {
        spdlog::debug("[PackageLoader] Component {} already registered", component.name);
        return true;
    }

    spdlog::warn("[PackageLoader] Component {} not found in registry or plugins", component.name);
    return false;
}

bool PackageLoader::loadSystemFromFile(const SystemInfo& system, const std::filesystem::path& basePath) {
    // Check built-in registry first
    if (hasSystem(system.name)) {
        spdlog::debug("[PackageLoader] System {} already registered", system.name);
        return true;
    }

    spdlog::warn("[PackageLoader] System {} not found in registry or plugins", system.name);
    return false;
}

} // namespace GameEngine
