#include "package_loader.h"
#include "../plugins/plugin_manager.h"
#include <spdlog/spdlog.h>

namespace GameEngine {

bool PackageLoader::loadPackageResources(const Package& package, const std::filesystem::path& packagePath) {
    spdlog::info("[PackageLoader] Loading resources for package: {}", package.getName());
    
    // Load plugin if package has one
    if (package.hasPluginInfo() && pluginManager) {
        const auto& pluginInfo = package.getPluginInfo();
        
        if (pluginInfo.autoload) {
            // Build plugin path
            auto pluginPath = packagePath / pluginInfo.library;
            
            spdlog::debug("[PackageLoader] Loading plugin: {}", pluginPath.string());
            
            if (!pluginManager->loadPlugin(pluginPath)) {
                lastError = "Failed to load plugin: " + pluginManager->getLastError();
                spdlog::error("[PackageLoader] {}", lastError);
                return false;
            }
            
            spdlog::info("[PackageLoader] Successfully loaded plugin from: {}", pluginPath.string());
        } else {
            spdlog::debug("[PackageLoader] Plugin autoload disabled for: {}", pluginInfo.library);
        }
    }
    
    // Process components from package.json
    for (const auto& componentInfo : package.getComponents()) {
        // Check if component is already registered (might have been registered by plugin)
        if (!hasComponent(componentInfo.name)) {
            spdlog::warn("[PackageLoader] Component {} not registered by plugin, skipping", componentInfo.name);
        } else {
            spdlog::debug("[PackageLoader] Component {} available", componentInfo.name);
        }
    }
    
    // Process systems from package.json
    for (const auto& systemInfo : package.getSystems()) {
        // Check if system is already registered (might have been registered by plugin)
        if (!hasSystem(systemInfo.name)) {
            spdlog::warn("[PackageLoader] System {} not registered by plugin, skipping", systemInfo.name);
        } else {
            spdlog::debug("[PackageLoader] System {} available", systemInfo.name);
        }
    }
    
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

} // namespace GameEngine