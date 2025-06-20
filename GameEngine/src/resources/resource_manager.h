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

protected:    
    // Default texture management
    std::unique_ptr<Texture2D> defaultTexture;
    mutable std::mutex defaultTextureMutex;
    std::atomic<bool> defaultTextureInitialized{false};
    std::atomic<bool> defaultTextureInitAttempted{false};
    
private:
    
    // Flags
    std::atomic<bool> silentMode{false};
    std::atomic<bool> headlessMode{false};
    std::atomic<bool> rayLibInitialized{false};

protected:    
    virtual void createDefaultTextureThreadSafe();
    
private:
    void createDummyTexture();
    void createRealTexture();
    void createEmergencyFallbackTexture() noexcept;

public:
    ResourceManager();
    virtual ~ResourceManager();
    
    void setSilentMode(bool silent) noexcept { silentMode.store(silent); }
    void setHeadlessMode(bool headless) noexcept { headlessMode.store(headless); }
    void setRayLibInitialized(bool initialized) noexcept { rayLibInitialized.store(initialized); }

    Texture2D* loadTexture(const std::string& path, const std::string& name);
    Sound* loadSound(const std::string& path, const std::string& name);
    
    Texture2D* getTexture(const std::string& name);
    Sound* getSound(const std::string& name);
    
    // Public access to default texture
    Texture2D& getDefaultTexture();
    
    void unloadAll();
    void clearAll(); // Alias for unloadAll for backwards compatibility
    void unloadTexture(const std::string& name);
    void unloadSound(const std::string& name);
    
    // Diagnostic methods
    size_t getLoadedTexturesCount() const noexcept { 
        std::shared_lock<std::shared_mutex> lock(resourceMutex);
        return textures.size(); 
    }
    size_t getUniqueTexturesCount() const noexcept;
};