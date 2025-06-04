#pragma once

#include <string>
#include <nlohmann/json.hpp>

class Config {
private:
    static inline nlohmann::json configData;
    static inline std::string configPath;
    static inline bool isLoaded = false;

public:
    // Load configuration from JSON file
    static bool load(const std::string& path);
    
    // Get value by key using dot notation (e.g., "window.width")
    static nlohmann::json get(const std::string& key, const nlohmann::json& defaultValue = {});
    
    // Type-specific getters
    static int getInt(const std::string& key, int defaultValue = 0);
    static float getFloat(const std::string& key, float defaultValue = 0.0f);
    static std::string getString(const std::string& key, const std::string& defaultValue = "");
    static bool getBool(const std::string& key, bool defaultValue = false);
    
    // Get array values
    template<typename T>
    static std::vector<T> getArray(const std::string& key, const std::vector<T>& defaultValue = {}) {
        try {
            auto value = get(key);
            if (value.is_array()) {
                return value.get<std::vector<T>>();
            }
        } catch (...) {}
        return defaultValue;
    }
    
    // Set value (runtime only, doesn't save to file)
    static void set(const std::string& key, const nlohmann::json& value);
    
    // Reload configuration from file
    static void reload();
    
    // Check if configuration is loaded
    static bool isConfigLoaded() { return isLoaded; }
    
    // Get the entire config object
    static const nlohmann::json& getConfig() { return configData; }
    
private:
    // Helper to parse dot notation keys
    static nlohmann::json* navigateToKey(const std::string& key, bool createPath = false);
};