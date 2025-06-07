#include "resource_manager.h"
#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>

ResourceManager::ResourceManager() {
    createDefaultTexture();
}

ResourceManager::~ResourceManager() {
    // Only unload texture if it was actually created by RayLib
    if (!headlessMode && rayLibInitialized && defaultTexture.id > 0) {
        UnloadTexture(defaultTexture);
    }
    unloadAll();
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
    if (textures.find(name) != textures.end()) {
        if (!silentMode) {
            spdlog::info("[ResourceManager] Texture '{}' already loaded.", name);
        }
        return &textures[name];
    }

    // In headless mode, always return default texture
    if (headlessMode) {
        textures[name] = defaultTexture;
        if (!silentMode) {
            spdlog::info("[ResourceManager] Headless mode: using dummy texture for '{}'", name);
        }
        return &textures[name];
    }

    if (!std::filesystem::exists(path)) {
        if (!silentMode) {
            spdlog::warn("[ResourceManager] Texture file not found: {}", path);
            spdlog::warn("[ResourceManager] Using default texture for '{}'", name);
        }
        // Store default texture with this name so subsequent calls return the same pointer
        textures[name] = defaultTexture;
        return &textures[name];
    }

    Texture2D texture = LoadTexture(path.c_str());
    if (texture.id == 0) {
        if (!silentMode) {
            spdlog::warn("[ResourceManager] Failed to load texture: {}", path);
            spdlog::warn("[ResourceManager] Using default texture for '{}'", name);
        }
        // Store default texture with this name so subsequent calls return the same pointer
        textures[name] = defaultTexture;
        return &textures[name];
    }

    textures[name] = texture;
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
    // Store default texture with this name so subsequent calls return the same pointer
    textures[name] = defaultTexture;
    return &textures[name];
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
        // Don't unload if it's the default texture
        if (texture.id != defaultTexture.id) {
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
        // Don't actually unload if it's the default texture (just remove from map)
        if (it->second.id != defaultTexture.id) {
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