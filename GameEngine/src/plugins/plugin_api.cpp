#include "plugin_api.h"
#include "../packages/package_loader.h"
#include <spdlog/spdlog.h>

namespace GameEngine {
void PluginAPI::registerComponent(const std::string& name, ComponentFactory factory) {
    spdlog::info("[PluginAPI] Registering component: {}", name);
    if (packageLoader) {
        packageLoader->registerComponent(name, factory);
    }
}

void PluginAPI::registerSystem(const std::string& name, SystemFactory factory) {
    spdlog::info("[PluginAPI] Registering system: {}", name);
    if (packageLoader) {
        packageLoader->registerSystem(name, factory);
    }
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
