#include "resource_manager.h"
#include <iostream>
#include <filesystem>
#include <unordered_set>
#include <chrono>
#include <spdlog/spdlog.h>

ResourceManager::ResourceManager() {
    // Default texture will be created on first use via std::once_flag
}

ResourceManager::~ResourceManager() {
    unloadAll();
    // Default texture is managed by unique_ptr, will be automatically cleaned up
}

void ResourceManager::createDummyTexture() {
    defaultTexture = std::make_unique<Texture2D>();
    defaultTexture->id = 0;
    defaultTexture->width = 64;
    defaultTexture->height = 64;
    defaultTexture->mipmaps = 1;
    defaultTexture->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
}

void ResourceManager::createRealTexture() {
    // Create real texture - only called when RayLib is initialized
    const int size = 64;
    const int checkSize = 8;
    
    // Create image for pink-black checkerboard
    Image img = GenImageChecked(size, size, checkSize, checkSize, MAGENTA, BLACK);
    
    // Create texture from image
    defaultTexture = std::make_unique<Texture2D>();
    *defaultTexture = LoadTextureFromImage(img);
    UnloadImage(img);
    
    if (!silentMode.load()) {
        spdlog::info("[ResourceManager] Created default texture (64x64 pink-black checkerboard)");
    }
}

void ResourceManager::createDefaultTextureThreadSafe() {
    try {
        if (headlessMode.load()) {
            createDummyTexture();
            if (!silentMode.load()) {
                spdlog::info("[ResourceManager] Created dummy texture for headless mode");
            }
        } else if (!rayLibInitialized.load()) {
            createDummyTexture();
            if (!silentMode.load()) {
                spdlog::info("[ResourceManager] Created dummy texture (RayLib not initialized)");
            }
        } else {
            createRealTexture();
        }
    } catch (const std::exception& e) {
        spdlog::error("[ResourceManager] Failed to create default texture: {}", e.what());
        // Emergency fallback - create dummy texture
        try {
            createDummyTexture();
            spdlog::warn("[ResourceManager] Using dummy texture as fallback");
        } catch (...) {
            spdlog::error("[ResourceManager] Failed to create even dummy texture - critical error");
            throw;
        }
    }
}

Texture2D& ResourceManager::getDefaultTexture() {
    // Double-checked locking pattern for thread-safe lazy initialization
    if (!defaultTextureInitialized.load(std::memory_order_acquire)) {
        std::lock_guard<std::mutex> lock(defaultTextureMutex);
        
        // Check again inside the lock
        if (!defaultTextureInitialized.load(std::memory_order_relaxed)) {
            createDefaultTextureThreadSafe();
            defaultTextureInitialized.store(true, std::memory_order_release);
        }
    }
    
    // Emergency safety check - should never happen with proper initialization
    if (!defaultTexture) {
        spdlog::error("[ResourceManager] Default texture is null after initialization - critical error");
        throw std::runtime_error("[ResourceManager] Default texture initialization failed");
    }
    
    return *defaultTexture;
}

Texture2D* ResourceManager::loadTexture(const std::string& path, const std::string& name) {
    // Simple headless check
    if (headlessMode.load()) {
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Headless mode: using dummy texture for '{}'", name);
        }
        return &getDefaultTexture();
    }
    
    // Thread-safe check if already loaded (read lock)
    {
        std::shared_lock<std::shared_mutex> lock(resourceMutex);
        auto it = textures.find(name);
        if (it != textures.end()) {
            if (!silentMode.load()) {
                spdlog::info("[ResourceManager] Texture '{}' already loaded.", name);
            }
            return &it->second;
        }
    }

    // Load texture WITHOUT holding any locks
    if (!std::filesystem::exists(path)) {
        if (!silentMode.load()) {
            spdlog::warn("[ResourceManager] Texture file not found: {}", path);
            spdlog::warn("[ResourceManager] Using default texture for '{}'", name);
        }
        return &getDefaultTexture();
    }

    Texture2D texture = LoadTexture(path.c_str());
    if (texture.id == 0) {
        if (!silentMode.load()) {
            spdlog::warn("[ResourceManager] Failed to load texture: {}", path);
            spdlog::warn("[ResourceManager] Using default texture for '{}'", name);
        }
        return &getDefaultTexture();
    }

    // Store texture with double-check pattern (write lock)
    {
        std::unique_lock<std::shared_mutex> lock(resourceMutex);
        
        // Double-check: another thread might have loaded it while we were loading
        auto it = textures.find(name);
        if (it != textures.end()) {
            // Another thread loaded it first, unload our copy and return existing
            UnloadTexture(texture);
            if (!silentMode.load()) {
                spdlog::info("[ResourceManager] Texture '{}' was loaded by another thread.", name);
            }
            return &it->second;
        }
        
        // We're first to load it
        textures[name] = std::move(texture);
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Loaded texture '{}' from: {}", name, path);
        }
        return &textures[name];
    }
}

Sound* ResourceManager::loadSound(const std::string& path, const std::string& name) {
    // Thread-safe check if already loaded (read lock)
    {
        std::shared_lock<std::shared_mutex> lock(resourceMutex);
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

    // Thread-safe sound storage (write lock)
    {
        std::unique_lock<std::shared_mutex> lock(resourceMutex);
        sounds[name] = sound;
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Loaded sound '{}' from: {}", name, path);
        }
        return &sounds[name];
    }
}

Texture2D* ResourceManager::getTexture(const std::string& name) {
    // Thread-safe texture lookup (read lock)
    {
        std::shared_lock<std::shared_mutex> lock(resourceMutex);
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
    return &getDefaultTexture();
}

Sound* ResourceManager::getSound(const std::string& name) {
    // Thread-safe sound lookup (read lock)
    std::shared_lock<std::shared_mutex> lock(resourceMutex);
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
    
    // Thread-safe resource unloading (write lock)
    std::unique_lock<std::shared_mutex> lock(resourceMutex);
    
    // Unload textures
    for (auto& [name, texture] : textures) {
        if (!headlessMode.load() && rayLibInitialized.load() && texture.id > 0) {
            UnloadTexture(texture);
        }
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Unloaded texture: {}", name);
        }
    }
    textures.clear();
    
    // Unload sounds
    for (auto& [name, sound] : sounds) {
        UnloadSound(sound);
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Unloaded sound: {}", name);
        }
    }
    sounds.clear();
}

void ResourceManager::unloadTexture(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(resourceMutex);
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
    std::unique_lock<std::shared_mutex> lock(resourceMutex);
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
    // Thread-safe texture count (read lock)
    std::shared_lock<std::shared_mutex> lock(resourceMutex);
    return textures.size();
}