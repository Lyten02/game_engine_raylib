#include "package_loader.h"
#include <spdlog/spdlog.h>

namespace GameEngine {

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