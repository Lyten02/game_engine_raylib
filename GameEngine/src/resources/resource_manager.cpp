#include "resource_manager.h"
#include <iostream>
#include <filesystem>
#include <unordered_set>
#include <chrono>
#include <spdlog/spdlog.h>

// Static member definitions for default texture
static Texture2D g_defaultTexture{0};
static bool g_defaultTextureInitialized = false;
static std::mutex g_defaultTextureMutex;

// Simple cleanup function to be called before RayLib shutdown
void cleanupDefaultTexture() {
    std::lock_guard<std::mutex> lock(g_defaultTextureMutex);
    if (g_defaultTextureInitialized && g_defaultTexture.id > 0) {
        UnloadTexture(g_defaultTexture);
        g_defaultTexture.id = 0;
        g_defaultTextureInitialized = false;
    }
}

ResourceManager::ResourceManager() {
    // Don't initialize default texture in constructor
    // It will be created on first use
}

ResourceManager::~ResourceManager() {
    unloadAll();
    // Don't touch default texture here - it's global and may be used by other instances
}

void ResourceManager::ensureDefaultTexture() {
    // Simple check without complex locking
    if (g_defaultTextureInitialized) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(g_defaultTextureMutex);
    // Double-check after acquiring lock
    if (!g_defaultTextureInitialized) {
        createDefaultTexture();
        g_defaultTextureInitialized = true;
    }
}

void ResourceManager::createDefaultTexture() {
    // Simple headless check
    if (headlessMode.load()) {
        // Create dummy texture for headless mode
        g_defaultTexture.id = 0;
        g_defaultTexture.width = 64;
        g_defaultTexture.height = 64;
        g_defaultTexture.mipmaps = 1;
        g_defaultTexture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Created dummy texture for headless mode");
        }
        return;
    }
    
    // For graphics mode, check if RayLib is ready
    if (!rayLibInitialized.load()) {
        // Create dummy texture if RayLib not ready
        g_defaultTexture.id = 0;
        g_defaultTexture.width = 64;
        g_defaultTexture.height = 64;
        g_defaultTexture.mipmaps = 1;
        g_defaultTexture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Created dummy texture (RayLib not initialized)");
        }
        return;
    }
    
    // Create real texture
    const int size = 64;
    const int checkSize = 8;
    
    // Create image for pink-black checkerboard
    Image img = GenImageChecked(size, size, checkSize, checkSize, MAGENTA, BLACK);
    
    // Create texture from image
    g_defaultTexture = LoadTextureFromImage(img);
    UnloadImage(img);
    
    if (!silentMode.load()) {
        spdlog::info("[ResourceManager] Created default texture (64x64 pink-black checkerboard)");
    }
}

Texture2D* ResourceManager::loadTexture(const std::string& path, const std::string& name) {
    auto start = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(2);
    
    // Simple headless check
    if (headlessMode.load()) {
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Headless mode: using dummy texture for '{}'", name);
        }
        return &getDefaultTexture();
    }
    
    // Thread-safe check if already loaded
    {
        std::lock_guard<std::mutex> lock(texturesMutex);
        
        // Check timeout
        if (std::chrono::steady_clock::now() - start > timeout) {
            spdlog::warn("[ResourceManager] loadTexture operation timed out for '{}'", name);
            return &getDefaultTexture();
        }
        
        auto it = textures.find(name);
        if (it != textures.end()) {
            if (!silentMode.load()) {
                spdlog::info("[ResourceManager] Texture '{}' already loaded.", name);
            }
            return &it->second;
        }
    }

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

    // Thread-safe texture storage
    {
        std::lock_guard<std::mutex> lock(texturesMutex);
        
        // Check timeout again before storing
        if (std::chrono::steady_clock::now() - start > timeout) {
            spdlog::warn("[ResourceManager] loadTexture storage timed out for '{}'", name);
            UnloadTexture(texture);
            return &getDefaultTexture();
        }
        
        textures[name] = std::move(texture);
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Loaded texture '{}' from: {}", name, path);
        }
        return &textures[name];
    }
}

Sound* ResourceManager::loadSound(const std::string& path, const std::string& name) {
    // Thread-safe check if already loaded
    {
        std::lock_guard<std::mutex> lock(soundsMutex);
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

    // Thread-safe sound storage
    {
        std::lock_guard<std::mutex> lock(soundsMutex);
        sounds[name] = sound;
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Loaded sound '{}' from: {}", name, path);
        }
        return &sounds[name];
    }
}

Texture2D* ResourceManager::getTexture(const std::string& name) {
    // Thread-safe texture lookup
    {
        std::lock_guard<std::mutex> lock(texturesMutex);
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
    ensureDefaultTexture();
    return &getDefaultTexture();
}

Sound* ResourceManager::getSound(const std::string& name) {
    // Thread-safe sound lookup
    std::lock_guard<std::mutex> lock(soundsMutex);
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
    
    // Thread-safe texture unloading
    {
        std::lock_guard<std::mutex> lock(texturesMutex);
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

    // Thread-safe sound unloading
    {
        std::lock_guard<std::mutex> lock(soundsMutex);
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
    std::lock_guard<std::mutex> lock(texturesMutex);
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
    std::lock_guard<std::mutex> lock(soundsMutex);
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
    // Thread-safe texture count
    std::lock_guard<std::mutex> lock(texturesMutex);
    return textures.size();
}

Texture2D& ResourceManager::getDefaultTexture() {
    // Use the global texture with simple locking
    std::lock_guard<std::mutex> lock(g_defaultTextureMutex);
    if (!g_defaultTextureInitialized) {
        // Create a minimal dummy texture for safety
        g_defaultTexture.id = 0;
        g_defaultTexture.width = 64;
        g_defaultTexture.height = 64;
        g_defaultTexture.mipmaps = 1;
        g_defaultTexture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        g_defaultTextureInitialized = true;
    }
    return g_defaultTexture;
}