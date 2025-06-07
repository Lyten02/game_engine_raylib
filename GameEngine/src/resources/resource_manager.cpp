#include "resource_manager.h"
#include <iostream>
#include <filesystem>
#include <unordered_set>
#include <spdlog/spdlog.h>

ResourceManager::ResourceManager() {
    // Don't create default texture yet - will be created lazily
    defaultTexture = {0, 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};  // Dummy values
}

ResourceManager::~ResourceManager() {
    unloadAll();
    // Only unload texture if it was actually created by RayLib
    if (!headlessMode && rayLibInitialized && defaultTextureInitialized.load() && defaultTexture.id > 0) {
        UnloadTexture(defaultTexture);
    }
}

void ResourceManager::ensureDefaultTexture() {
    // Thread-safe lazy initialization using std::call_once
    std::call_once(defaultTextureFlag, [this]() {
        createDefaultTexture();
        defaultTextureInitialized.store(true);
    });
}

void ResourceManager::createDefaultTexture() {
    // Check if RayLib is ready to work
    if (headlessMode || !rayLibInitialized || !IsWindowReady()) {
        // Create dummy texture for headless mode
        defaultTexture = {0, 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
        if (!silentMode) {
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
    
    if (!silentMode) {
        spdlog::info("[ResourceManager] Created default texture (64x64 pink-black checkerboard)");
    }
}

Texture2D* ResourceManager::loadTexture(const std::string& path, const std::string& name) {
    // Check if already loaded
    auto it = textures.find(name);
    if (it != textures.end()) {
        if (!silentMode) {
            spdlog::info("[ResourceManager] Texture '{}' already loaded.", name);
        }
        return &it->second;
    }

    // In headless mode, always return default texture without storing
    if (headlessMode) {
        if (!silentMode) {
            spdlog::info("[ResourceManager] Headless mode: using dummy texture for '{}'", name);
        }
        ensureDefaultTexture();
        return &defaultTexture;
    }

    if (!std::filesystem::exists(path)) {
        if (!silentMode) {
            spdlog::warn("[ResourceManager] Texture file not found: {}", path);
            spdlog::warn("[ResourceManager] Using default texture for '{}'", name);
        }
        // Return default texture without storing in map
        ensureDefaultTexture();
        return &defaultTexture;
    }

    Texture2D texture = LoadTexture(path.c_str());
    if (texture.id == 0) {
        if (!silentMode) {
            spdlog::warn("[ResourceManager] Failed to load texture: {}", path);
            spdlog::warn("[ResourceManager] Using default texture for '{}'", name);
        }
        // Return default texture without storing in map
        ensureDefaultTexture();
        return &defaultTexture;
    }

    // Store texture directly in map using move semantics
    textures[name] = std::move(texture);
    if (!silentMode) {
        spdlog::info("[ResourceManager] Loaded texture '{}' from: {}", name, path);
    }
    return &textures[name];
}

Sound* ResourceManager::loadSound(const std::string& path, const std::string& name) {
    if (sounds.find(name) != sounds.end()) {
        if (!silentMode) {
            spdlog::info("[ResourceManager] Sound '{}' already loaded.", name);
        }
        return &sounds[name];
    }

    if (!std::filesystem::exists(path)) {
        if (!silentMode) {
            spdlog::error("[ResourceManager] Sound file not found: {}", path);
        }
        return nullptr;
    }

    Sound sound = LoadSound(path.c_str());
    if (sound.frameCount == 0) {
        if (!silentMode) {
            spdlog::error("[ResourceManager] Failed to load sound: {}", path);
        }
        return nullptr;
    }

    sounds[name] = sound;
    if (!silentMode) {
        spdlog::info("[ResourceManager] Loaded sound '{}' from: {}", name, path);
    }
    return &sounds[name];
}

Texture2D* ResourceManager::getTexture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        return &it->second;
    }
    if (!silentMode) {
        spdlog::warn("[ResourceManager] Texture '{}' not found.", name);
        spdlog::warn("[ResourceManager] Using default texture for '{}'", name);
    }
    // Return default texture without storing in map
    ensureDefaultTexture();
    return &defaultTexture;
}

Sound* ResourceManager::getSound(const std::string& name) {
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        return &it->second;
    }
    if (!silentMode) {
        spdlog::warn("[ResourceManager] Sound '{}' not found.", name);
    }
    return nullptr;
}

void ResourceManager::unloadAll() {
    if (!silentMode) {
        spdlog::info("[ResourceManager] Unloading all resources...");
    }
    
    for (auto& [name, texture] : textures) {
        if (!headlessMode && rayLibInitialized && texture.id > 0) {
            UnloadTexture(texture);
        }
        if (!silentMode) {
            spdlog::info("[ResourceManager] Unloaded texture: {}", name);
        }
    }
    textures.clear();

    for (auto& [name, sound] : sounds) {
        UnloadSound(sound);
        if (!silentMode) {
            spdlog::info("[ResourceManager] Unloaded sound: {}", name);
        }
    }
    sounds.clear();
}

void ResourceManager::unloadTexture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        if (!headlessMode && rayLibInitialized && it->second.id > 0) {
            UnloadTexture(it->second);
        }
        textures.erase(it);
        if (!silentMode) {
            spdlog::info("[ResourceManager] Unloaded texture: {}", name);
        }
    } else {
        if (!silentMode) {
            spdlog::warn("[ResourceManager] Cannot unload texture '{}' - not found.", name);
        }
    }
}

void ResourceManager::unloadSound(const std::string& name) {
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        UnloadSound(it->second);
        sounds.erase(it);
        if (!silentMode) {
            spdlog::info("[ResourceManager] Unloaded sound: {}", name);
        }
    } else {
        if (!silentMode) {
            spdlog::warn("[ResourceManager] Cannot unload sound '{}' - not found.", name);
        }
    }
}

size_t ResourceManager::getUniqueTexturesCount() const {
    // With the new design, each texture in the map is unique
    // We just need to count actual loaded textures (not counting references to default)
    return textures.size();
}