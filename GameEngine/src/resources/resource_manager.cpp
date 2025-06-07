#include "resource_manager.h"
#include <iostream>
#include <filesystem>
#include <unordered_set>
#include <spdlog/spdlog.h>

// No static member definitions needed - DefaultTextureManager handles it internally

ResourceManager::ResourceManager() {
    // Default texture will be created lazily when first needed
}

ResourceManager::~ResourceManager() {
    unloadAll();
    // DefaultTextureManager handles cleanup via RAII
}

void ResourceManager::ensureDefaultTexture(bool headless, bool rayLibInit, bool silent) {
    // Thread-safe lazy initialization using std::call_once
    auto& manager = DefaultTextureManager::getInstance();
    std::call_once(manager.getInitFlag(), [headless, rayLibInit, silent]() {
        createDefaultTexture(headless, rayLibInit, silent);
        DefaultTextureManager::getInstance().setInitialized(true);
    });
}

void ResourceManager::createDefaultTexture(bool headless, bool rayLibInit, bool silent) {
    auto& manager = DefaultTextureManager::getInstance();
    Texture2D& defaultTexture = manager.getTexture();
    
    // Check if RayLib is ready to work
    if (headless || !rayLibInit || !IsWindowReady()) {
        // Create safe dummy texture for headless mode
        // Using id=0 to indicate it's not a real OpenGL texture
        defaultTexture.id = 0;
        defaultTexture.width = 64;
        defaultTexture.height = 64;
        defaultTexture.mipmaps = 1;
        defaultTexture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        
        if (!silent) {
            spdlog::info("[ResourceManager] Created dummy texture for headless mode");
        }
        return;
    }
    
    // Existing code for creating real texture
    const int size = 64;
    const int checkSize = 8;
    
    // Create image for pink-black checkerboard
    Image img = GenImageChecked(size, size, checkSize, checkSize, MAGENTA, BLACK);
    
    // Create texture from image
    defaultTexture = LoadTextureFromImage(img);
    UnloadImage(img);
    
    if (!silent) {
        spdlog::info("[ResourceManager] Created default texture (64x64 pink-black checkerboard)");
    }
    // No need for atexit - DefaultTextureManager destructor handles cleanup
}

Texture2D* ResourceManager::loadTexture(const std::string& path, const std::string& name) {
    // Thread-safe check if already loaded - using shared_lock for read
    {
        std::shared_lock<std::shared_mutex> lock(texturesMutex);
        auto it = textures.find(name);
        if (it != textures.end()) {
            if (!silentMode.load()) {
                spdlog::info("[ResourceManager] Texture '{}' already loaded.", name);
            }
            return &it->second;
        }
    }

    // In headless mode, always return default texture without storing
    if (headlessMode.load()) {
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Headless mode: using dummy texture for '{}'", name);
        }
        ensureDefaultTexture(headlessMode.load(), rayLibInitialized.load(), silentMode.load());
        return &DefaultTextureManager::getInstance().getTexture();
    }

    if (!std::filesystem::exists(path)) {
        if (!silentMode.load()) {
            spdlog::warn("[ResourceManager] Texture file not found: {}", path);
            spdlog::warn("[ResourceManager] Using default texture for '{}'", name);
        }
        // Return default texture without storing in map
        ensureDefaultTexture(headlessMode.load(), rayLibInitialized.load(), silentMode.load());
        return &DefaultTextureManager::getInstance().getTexture();
    }

    Texture2D texture = LoadTexture(path.c_str());
    if (texture.id == 0) {
        if (!silentMode.load()) {
            spdlog::warn("[ResourceManager] Failed to load texture: {}", path);
            spdlog::warn("[ResourceManager] Using default texture for '{}'", name);
        }
        // Return default texture without storing in map
        ensureDefaultTexture(headlessMode.load(), rayLibInitialized.load(), silentMode.load());
        return &DefaultTextureManager::getInstance().getTexture();
    }

    // Thread-safe texture storage - exclusive lock for write
    {
        std::lock_guard<std::shared_mutex> lock(texturesMutex);
        textures[name] = std::move(texture);
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Loaded texture '{}' from: {}", name, path);
        }
        return &textures[name];
    }
}

Sound* ResourceManager::loadSound(const std::string& path, const std::string& name) {
    // Thread-safe check if already loaded - using shared_lock for read
    {
        std::shared_lock<std::shared_mutex> lock(soundsMutex);
        auto it = sounds.find(name);
        if (it != sounds.end()) {
            if (!silentMode.load()) {
                spdlog::info("[ResourceManager] Sound '{}' already loaded.", name);
            }
            return &it->second;
        }
    }

    if (!std::filesystem::exists(path)) {
        if (!silentMode.load()) {
            spdlog::error("[ResourceManager] Sound file not found: {}", path);
        }
        return nullptr;
    }

    Sound sound = LoadSound(path.c_str());
    if (sound.frameCount == 0) {
        if (!silentMode.load()) {
            spdlog::error("[ResourceManager] Failed to load sound: {}", path);
        }
        return nullptr;
    }

    // Thread-safe sound storage - exclusive lock for write
    {
        std::lock_guard<std::shared_mutex> lock(soundsMutex);
        sounds[name] = sound;
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Loaded sound '{}' from: {}", name, path);
        }
        return &sounds[name];
    }
}

Texture2D* ResourceManager::getTexture(const std::string& name) {
    // Thread-safe texture lookup - using shared_lock for read
    {
        std::shared_lock<std::shared_mutex> lock(texturesMutex);
        auto it = textures.find(name);
        if (it != textures.end()) {
            return &it->second;
        }
    }
    
    if (!silentMode.load()) {
        spdlog::warn("[ResourceManager] Texture '{}' not found.", name);
        spdlog::warn("[ResourceManager] Using default texture for '{}'", name);
    }
    // Return default texture without storing in map
    ensureDefaultTexture(headlessMode.load(), rayLibInitialized.load(), silentMode.load());
    return &DefaultTextureManager::getInstance().getTexture();
}

Sound* ResourceManager::getSound(const std::string& name) {
    // Thread-safe sound lookup - using shared_lock for read
    std::shared_lock<std::shared_mutex> lock(soundsMutex);
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        return &it->second;
    }
    if (!silentMode.load()) {
        spdlog::warn("[ResourceManager] Sound '{}' not found.", name);
    }
    return nullptr;
}

void ResourceManager::unloadAll() {
    if (!silentMode.load()) {
        spdlog::info("[ResourceManager] Unloading all resources...");
    }
    
    // Thread-safe texture unloading - exclusive lock for write
    {
        std::lock_guard<std::shared_mutex> lock(texturesMutex);
        for (auto& [name, texture] : textures) {
            if (!headlessMode.load() && rayLibInitialized.load() && texture.id > 0) {
                UnloadTexture(texture);
            }
            if (!silentMode.load()) {
                spdlog::info("[ResourceManager] Unloaded texture: {}", name);
            }
        }
        textures.clear();
    }

    // Thread-safe sound unloading - exclusive lock for write
    {
        std::lock_guard<std::shared_mutex> lock(soundsMutex);
        for (auto& [name, sound] : sounds) {
            UnloadSound(sound);
            if (!silentMode.load()) {
                spdlog::info("[ResourceManager] Unloaded sound: {}", name);
            }
        }
        sounds.clear();
    }
}

void ResourceManager::unloadTexture(const std::string& name) {
    std::lock_guard<std::shared_mutex> lock(texturesMutex);
    auto it = textures.find(name);
    if (it != textures.end()) {
        if (!headlessMode.load() && rayLibInitialized.load() && it->second.id > 0) {
            UnloadTexture(it->second);
        }
        textures.erase(it);
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Unloaded texture: {}", name);
        }
    } else {
        if (!silentMode.load()) {
            spdlog::warn("[ResourceManager] Cannot unload texture '{}' - not found.", name);
        }
    }
}

void ResourceManager::unloadSound(const std::string& name) {
    std::lock_guard<std::shared_mutex> lock(soundsMutex);
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        UnloadSound(it->second);
        sounds.erase(it);
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Unloaded sound: {}", name);
        }
    } else {
        if (!silentMode.load()) {
            spdlog::warn("[ResourceManager] Cannot unload sound '{}' - not found.", name);
        }
    }
}

size_t ResourceManager::getUniqueTexturesCount() const {
    // Thread-safe texture count - using shared_lock for read
    std::shared_lock<std::shared_mutex> lock(texturesMutex);
    return textures.size();
}