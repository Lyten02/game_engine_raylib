#include "package.h"

namespace GameEngine {
Package::Package(const std::string& name, const std::string& version)
    : name(name), version(version) {
}

void Package::addDependency(const std::string& depName, const std::string& depVersion) {
    dependencies.push_back({depName, depVersion});
}

void Package::addComponent(const ComponentInfo& component) {
    components.push_back(component);
}

void Package::addSystem(const SystemInfo& system) {
    systems.push_back(system);
}

} // namespace GameEngine
