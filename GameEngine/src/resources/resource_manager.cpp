#include "resource_manager.h"
#include <iostream>
#include <filesystem>

ResourceManager::ResourceManager() {
    createDefaultTexture();
}

ResourceManager::~ResourceManager() {
    UnloadTexture(defaultTexture);
    unloadAll();
}

void ResourceManager::createDefaultTexture() {
    const int size = 64;
    const int checkSize = 8;
    
    // Create image for pink-black checkerboard
    Image img = GenImageChecked(size, size, checkSize, checkSize, MAGENTA, BLACK);
    
    // Create texture from image
    defaultTexture = LoadTextureFromImage(img);
    UnloadImage(img);
    
    if (!silentMode) {
        std::cout << "[ResourceManager] Created default texture (64x64 pink-black checkerboard)" << std::endl;
    }
}

Texture2D* ResourceManager::loadTexture(const std::string& path, const std::string& name) {
    if (textures.find(name) != textures.end()) {
        if (!silentMode) {
            std::cout << "[ResourceManager] Texture '" << name << "' already loaded." << std::endl;
        }
        return &textures[name];
    }

    if (!std::filesystem::exists(path)) {
        std::cerr << "[ResourceManager] WARNING: Texture file not found: " << path << std::endl;
        std::cerr << "[ResourceManager] Using default texture for '" << name << "'" << std::endl;
        // Store default texture with this name so subsequent calls return the same pointer
        textures[name] = defaultTexture;
        return &textures[name];
    }

    Texture2D texture = LoadTexture(path.c_str());
    if (texture.id == 0) {
        std::cerr << "[ResourceManager] WARNING: Failed to load texture: " << path << std::endl;
        std::cerr << "[ResourceManager] Using default texture for '" << name << "'" << std::endl;
        // Store default texture with this name so subsequent calls return the same pointer
        textures[name] = defaultTexture;
        return &textures[name];
    }

    textures[name] = texture;
    if (!silentMode) {
        std::cout << "[ResourceManager] Loaded texture '" << name << "' from: " << path << std::endl;
    }
    return &textures[name];
}

Sound* ResourceManager::loadSound(const std::string& path, const std::string& name) {
    if (sounds.find(name) != sounds.end()) {
        std::cout << "[ResourceManager] Sound '" << name << "' already loaded." << std::endl;
        return &sounds[name];
    }

    if (!std::filesystem::exists(path)) {
        std::cerr << "[ResourceManager] Error: Sound file not found: " << path << std::endl;
        return nullptr;
    }

    Sound sound = LoadSound(path.c_str());
    if (sound.frameCount == 0) {
        std::cerr << "[ResourceManager] Error: Failed to load sound: " << path << std::endl;
        return nullptr;
    }

    sounds[name] = sound;
    std::cout << "[ResourceManager] Loaded sound '" << name << "' from: " << path << std::endl;
    return &sounds[name];
}

Texture2D* ResourceManager::getTexture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        return &it->second;
    }
    std::cerr << "[ResourceManager] WARNING: Texture '" << name << "' not found." << std::endl;
    std::cerr << "[ResourceManager] Using default texture for '" << name << "'" << std::endl;
    // Store default texture with this name so subsequent calls return the same pointer
    textures[name] = defaultTexture;
    return &textures[name];
}

Sound* ResourceManager::getSound(const std::string& name) {
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        return &it->second;
    }
    std::cerr << "[ResourceManager] Warning: Sound '" << name << "' not found." << std::endl;
    return nullptr;
}

void ResourceManager::unloadAll() {
    if (!silentMode) {
        std::cout << "[ResourceManager] Unloading all resources..." << std::endl;
    }
    
    for (auto& [name, texture] : textures) {
        // Don't unload if it's the default texture
        if (texture.id != defaultTexture.id) {
            UnloadTexture(texture);
        }
        if (!silentMode) {
            std::cout << "[ResourceManager] Unloaded texture: " << name << std::endl;
        }
    }
    textures.clear();

    for (auto& [name, sound] : sounds) {
        UnloadSound(sound);
        if (!silentMode) {
            std::cout << "[ResourceManager] Unloaded sound: " << name << std::endl;
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
            std::cout << "[ResourceManager] Unloaded texture: " << name << std::endl;
        }
    } else {
        std::cerr << "[ResourceManager] Warning: Cannot unload texture '" << name << "' - not found." << std::endl;
    }
}

void ResourceManager::unloadSound(const std::string& name) {
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        UnloadSound(it->second);
        sounds.erase(it);
        std::cout << "[ResourceManager] Unloaded sound: " << name << std::endl;
    } else {
        std::cerr << "[ResourceManager] Warning: Cannot unload sound '" << name << "' - not found." << std::endl;
    }
}