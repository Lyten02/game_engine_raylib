#include "plugin_api.h"
#include "../packages/package_loader.h"
#include <spdlog/spdlog.h>

namespace GameEngine {

void PluginAPI::registerComponent(const std::string& name, ComponentFactory factory) {
    // For now, just log since PackageLoader is a stub
    spdlog::info("[PluginAPI] Registering component: {}", name);
    // TODO: packageLoader->registerComponent(name, factory);
}

void PluginAPI::registerSystem(const std::string& name, SystemFactory factory) {
    // For now, just log since PackageLoader is a stub
    spdlog::info("[PluginAPI] Registering system: {}", name);
    // TODO: packageLoader->registerSystem(name, factory);
}

void PluginAPI::log(const std::string& message) {
    spdlog::info("[Plugin] {}", message);
}

void PluginAPI::logError(const std::string& message) {
    spdlog::error("[Plugin] {}", message);
}

void PluginAPI::logWarning(const std::string& message) {
    spdlog::warn("[Plugin] {}", message);
}

} // namespace GameEngine