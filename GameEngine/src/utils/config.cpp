#include "config.h"
#include <fstream>
#include <spdlog/spdlog.h>
#include <filesystem>

bool Config::load(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            spdlog::error("Config::load - File not found: {}", path);
            return false;
        }
        
        std::ifstream file(path);
        if (!file.is_open()) {
            spdlog::error("Config::load - Failed to open file: {}", path);
            return false;
        }
        
        file >> configData;
        configPath = path;
        isLoaded = true;
        
        spdlog::info("Config::load - Configuration loaded from: {}", path);
        return true;
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error("Config::load - JSON parse error: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Config::load - Error loading config: {}", e.what());
        return false;
    }
}

nlohmann::json Config::get(const std::string& key, const nlohmann::json& defaultValue) {
    if (!isLoaded) {
        spdlog::warn("Config::get - Configuration not loaded, returning default value");
        return defaultValue;
    }
    
    try {
        nlohmann::json* result = navigateToKey(key);
        if (result) {
            return *result;
        }
    } catch (const std::exception& e) {
        spdlog::debug("Config::get - Key '{}' not found: {}", key, e.what());
    }
    
    return defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) {
    auto value = get(key);
    if (value.is_number_integer()) {
        return value.get<int>();
    }
    return defaultValue;
}

float Config::getFloat(const std::string& key, float defaultValue) {
    auto value = get(key);
    if (value.is_number()) {
        return value.get<float>();
    }
    return defaultValue;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) {
    auto value = get(key);
    if (value.is_string()) {
        return value.get<std::string>();
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) {
    auto value = get(key);
    if (value.is_boolean()) {
        return value.get<bool>();
    }
    return defaultValue;
}

void Config::set(const std::string& key, const nlohmann::json& value) {
    if (!isLoaded) {
        spdlog::warn("Config::set - Configuration not loaded");
        return;
    }
    
    try {
        nlohmann::json* target = navigateToKey(key, true);
        if (target) {
            *target = value;
            spdlog::debug("Config::set - Set '{}' to: {}", key, value.dump());
        }
    } catch (const std::exception& e) {
        spdlog::error("Config::set - Error setting key '{}': {}", key, e.what());
    }
}

void Config::reload() {
    if (configPath.empty()) {
        spdlog::warn("Config::reload - No config path set");
        return;
    }
    
    load(configPath);
}

nlohmann::json* Config::navigateToKey(const std::string& key, bool createPath) {
    if (key.empty()) {
        return &configData;
    }
    
    std::vector<std::string> parts;
    std::stringstream ss(key);
    std::string part;
    
    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }
    
    nlohmann::json* current = &configData;
    
    for (const auto& part : parts) {
        if (current->is_object()) {
            if (current->contains(part)) {
                current = &(*current)[part];
            } else if (createPath) {
                (*current)[part] = nlohmann::json::object();
                current = &(*current)[part];
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    }
    
    return current;
}