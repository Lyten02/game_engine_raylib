#pragma once

#include <unordered_map>
#include <string>
#include <memory>
// Removed shared_mutex - using simple mutex for thread safety
#include <mutex>
#include <atomic>
#include "raylib.h"

class ResourceManager {
private:
    std::unordered_map<std::string, Texture2D> textures;  // Store textures directly, not pointers
    std::unordered_map<std::string, Sound> sounds;
    
    // Thread safety - using simple mutex to avoid deadlocks
    mutable std::mutex texturesMutex;
    mutable std::mutex soundsMutex;
    
    // Simple static default texture management
    static Texture2D defaultTexture;
    static bool defaultTextureInitialized;
    static std::mutex defaultTextureMutex;
    
    // Flags
    std::atomic<bool> silentMode{false};
    std::atomic<bool> headlessMode{false};
    std::atomic<bool> rayLibInitialized{false};
    
    void createDefaultTexture();
    void ensureDefaultTexture();  // Simple initialization

public:
    ResourceManager();
    ~ResourceManager();
    
    void setSilentMode(bool silent) { silentMode.store(silent); }
    void setHeadlessMode(bool headless) { headlessMode.store(headless); }
    void setRayLibInitialized(bool initialized) { rayLibInitialized.store(initialized); }

    Texture2D* loadTexture(const std::string& path, const std::string& name);
    Sound* loadSound(const std::string& path, const std::string& name);
    
    Texture2D* getTexture(const std::string& name);
    Sound* getSound(const std::string& name);
    
    // Public access to default texture
    static Texture2D& getDefaultTexture();
    
    void unloadAll();
    void unloadTexture(const std::string& name);
    void unloadSound(const std::string& name);
    
    // Diagnostic methods
    size_t getLoadedTexturesCount() const { 
        std::lock_guard<std::mutex> lock(texturesMutex);
        return textures.size(); 
    }
    size_t getUniqueTexturesCount() const;
};