#include "config.h"
#include <fstream>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <sstream>

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
        
        if (!silentMode) {
            spdlog::info("Config::load - Configuration loaded from: {}", path);
        }
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
    if (!isLoaded || !isValidConfigKey(key)) {
        if (!isLoaded && !silentMode) {
            spdlog::warn("Config::get - Configuration not loaded, returning default value");
        }
        return defaultValue;
    }
    
    try {
        nlohmann::json* result = navigateToKey(key, false, MAX_KEY_DEPTH);
        if (result) {
            return *result;
        }
    } catch (const std::exception& e) {
        if (!silentMode) {
            spdlog::debug("Config::get - Key '{}' not found: {}", key, e.what());
        }
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
    if (!isLoaded || !isValidConfigKey(key)) {
        if (!isLoaded && !silentMode) {
            spdlog::warn("Config::set - Configuration not loaded");
        }
        return;
    }
    
    try {
        nlohmann::json* target = navigateToKey(key, true, MAX_KEY_DEPTH);
        if (target) {
            *target = value;
            if (!silentMode) {
                spdlog::debug("Config::set - Set '{}' to: {}", key, value.dump());
            }
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

bool Config::isValidConfigKey(const std::string& key) {
    if (key.empty() || key.length() > 100) return false;
    if (key.front() == '.' || key.back() == '.') return false;
    if (key.find("..") != std::string::npos) return false;
    return true;
}

std::vector<std::string> Config::parseKeyParts(const std::string& key) {
    std::vector<std::string> parts;
    std::stringstream ss(key);
    std::string part;
    
    while (std::getline(ss, part, '.')) {
        // Skip empty parts to prevent infinite loops
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    
    return parts;
}

nlohmann::json* Config::navigateToKey(const std::string& key, bool createPath, int maxDepth) {
    if (key.empty() || maxDepth <= 0) {
        return &configData;
    }
    
    auto parts = parseKeyParts(key);
    if (parts.empty() || parts.size() > static_cast<size_t>(MAX_KEY_DEPTH)) {
        return nullptr;
    }
    
    nlohmann::json* current = &configData;
    
    for (size_t i = 0; i < parts.size() && i < static_cast<size_t>(maxDepth); i++) {
        const auto& part = parts[i];
        
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