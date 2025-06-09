#include "config.h"
#include <fstream>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <set>

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
    if (!isLoaded) {
        if (!silentMode) {
            spdlog::warn("Config::get - Configuration not loaded, returning default value");
        }
        return defaultValue;
    }
    
    if (!isValidConfigKey(key)) {
        if (!silentMode) {
            spdlog::warn("Config::get - Invalid key format: {}", key);
        }
        return defaultValue;
    }
    
    try {
        nlohmann::json* result = navigateToKey(key, false);
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
    if (!isLoaded) {
        if (!silentMode) {
            spdlog::warn("Config::set - Configuration not loaded");
        }
        return;
    }
    
    if (!isValidConfigKey(key)) {
        if (!silentMode) {
            spdlog::warn("Config::set - Invalid key format: {}", key);
        }
        return;
    }
    
    try {
        nlohmann::json* target = navigateToKey(key, true);
        if (target) {
            *target = value;
            if (!silentMode) {
                spdlog::debug("Config::set - Set '{}' to: {}", key, value.dump());
            }
        } else {
            if (!silentMode) {
                spdlog::warn("Config::set - navigateToKey returned null for key: {}", key);
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
    
    // Check for invalid characters
    for (char c : key) {
        if (!std::isalnum(c) && c != '.' && c != '_') return false;
    }
    
    // Check maximum nesting depth
    // A key with N dots has N+1 parts/levels
    // We allow up to MAX_CONFIG_DEPTH levels, so we allow up to MAX_CONFIG_DEPTH-1 dots
    size_t dotCount = std::count(key.begin(), key.end(), '.');
    if (dotCount >= MAX_CONFIG_DEPTH) return false;
    
    return true;
}

std::vector<std::string> Config::parseKeyParts(const std::string& key) {
    std::vector<std::string> parts;
    
    std::stringstream ss(key);
    std::string part;
    
    while (std::getline(ss, part, '.')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    
    return parts;
}

nlohmann::json* Config::navigateToKey(const std::string& key, bool createPath, int maxDepth) {
    if (key.empty()) {
        return &configData;
    }
    
    // Validate key format first
    if (!isValidConfigKey(key)) {
        if (!silentMode) {
            spdlog::warn("Config::navigateToKey - Invalid key format: {}", key);
        }
        return nullptr;
    }
    
    auto parts = parseKeyParts(key);
    if (parts.empty()) {
        if (!silentMode) {
            spdlog::warn("Config::navigateToKey - Empty key parts: {}", key);
        }
        return nullptr;
    }
    
    // Single depth check here - removed from parseKeyParts
    if (parts.size() > static_cast<size_t>(maxDepth)) {
        if (!silentMode) {
            spdlog::warn("Config::navigateToKey - Key depth exceeds limit: {} (depth: {}, max: {})", 
                        key, parts.size(), maxDepth);
        }
        return nullptr;
    }
    
    // Debug logging
    if (!silentMode && parts.size() == 10) {
        spdlog::debug("Config::navigateToKey - Processing 10-level key: {}, parts: {}, maxDepth: {}", 
                     key, parts.size(), maxDepth);
    }
    
    nlohmann::json* current = &configData;
    std::set<const nlohmann::json*> visited; // Track visited nodes for circular reference detection
    visited.insert(current);
    
    // Ensure root is an object if creating paths
    if (createPath && !configData.is_object() && configData.is_null()) {
        configData = nlohmann::json::object();
    }
    
    for (size_t i = 0; i < parts.size(); i++) {
        const auto& part = parts[i];
        
        if (!current->is_object()) {
            if (!silentMode) {
                spdlog::debug("Config::navigateToKey - Current node is not an object at part {} of key {}", i, key);
            }
            return nullptr;
        }
        
        if (current->contains(part)) {
            nlohmann::json* next = &(*current)[part];
            
            // Check for circular reference
            if (visited.find(next) != visited.end()) {
                if (!silentMode) {
                    spdlog::error("Config::navigateToKey - Circular reference detected at key: {}", key);
                }
                return nullptr;
            }
            
            // If this is not the last part and the value exists but is not an object,
            // we can't continue navigating
            if (i < parts.size() - 1 && !next->is_object()) {
                if (!silentMode) {
                    spdlog::debug("Config::navigateToKey - Cannot navigate through non-object at part {} of key {}", part, key);
                }
                return nullptr;
            }
            
            current = next;
            visited.insert(current);
        } else if (createPath) {
            // Create the path
            if (i == parts.size() - 1) {
                // Last part - can be any type
                (*current)[part] = nlohmann::json{};
            } else {
                // Intermediate parts must be objects
                (*current)[part] = nlohmann::json::object();
            }
            current = &(*current)[part];
            visited.insert(current);
        } else {
            if (!silentMode && parts.size() >= 10) {
                spdlog::debug("Config::navigateToKey - Part '{}' not found at level {} in key {}", part, i, key);
            }
            return nullptr;
        }
    }
    
    return current;
}