#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <atomic>
#include "raylib.h"

class ResourceManager {
private:
    std::unordered_map<std::string, Texture2D> textures;  // Store textures directly, not pointers
    std::unordered_map<std::string, Sound> sounds;
    
    // Thread safety - using shared_mutex for better performance
    mutable std::shared_mutex resourceMutex;
    
    // Default texture management
    std::unique_ptr<Texture2D> defaultTexture;
    mutable std::mutex defaultTextureMutex;
    std::atomic<bool> defaultTextureInitialized{false};
    
    // Flags
    std::atomic<bool> silentMode{false};
    std::atomic<bool> headlessMode{false};
    std::atomic<bool> rayLibInitialized{false};
    
    void createDefaultTextureThreadSafe();
    void createDummyTexture();
    void createRealTexture();

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
    Texture2D& getDefaultTexture();
    
    void unloadAll();
    void unloadTexture(const std::string& name);
    void unloadSound(const std::string& name);
    
    // Diagnostic methods
    size_t getLoadedTexturesCount() const { 
        std::shared_lock<std::shared_mutex> lock(resourceMutex);
        return textures.size(); 
    }
    size_t getUniqueTexturesCount() const;
};