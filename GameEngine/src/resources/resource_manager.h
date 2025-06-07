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
    
    // Thread safety - using shared_mutex for better read performance
    mutable std::shared_mutex texturesMutex;
    mutable std::shared_mutex soundsMutex;
    
    // RAII wrapper for default texture to ensure proper cleanup
    class DefaultTextureManager {
    private:
        Texture2D texture{0};
        std::once_flag initFlag;
        std::atomic<bool> initialized{false};
        
    public:
        static DefaultTextureManager& getInstance() {
            static DefaultTextureManager instance;
            return instance;
        }
        
        Texture2D& getTexture() { return texture; }
        const Texture2D& getTexture() const { return texture; }
        std::once_flag& getInitFlag() { return initFlag; }
        bool isInitialized() const { return initialized.load(); }
        void setInitialized(bool value) { initialized.store(value); }
        
        ~DefaultTextureManager() {
            // Safe cleanup - destructor called during static destruction
            if (initialized.load() && texture.id > 0) {
                UnloadTexture(texture);
            }
        }
        
        // Delete copy/move constructors and assignment operators
        DefaultTextureManager(const DefaultTextureManager&) = delete;
        DefaultTextureManager& operator=(const DefaultTextureManager&) = delete;
        DefaultTextureManager(DefaultTextureManager&&) = delete;
        DefaultTextureManager& operator=(DefaultTextureManager&&) = delete;
        
    private:
        DefaultTextureManager() = default;
    };
    
    // Flags
    std::atomic<bool> silentMode{false};
    std::atomic<bool> headlessMode{false};
    std::atomic<bool> rayLibInitialized{false};
    
    static void createDefaultTexture(bool headless, bool rayLibInit, bool silent);
    static void ensureDefaultTexture(bool headless, bool rayLibInit, bool silent);  // Thread-safe lazy initialization

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
    
    void unloadAll();
    void unloadTexture(const std::string& name);
    void unloadSound(const std::string& name);
    
    // Diagnostic methods
    size_t getLoadedTexturesCount() const { 
        std::shared_lock<std::shared_mutex> lock(texturesMutex);
        return textures.size(); 
    }
    size_t getUniqueTexturesCount() const;
};