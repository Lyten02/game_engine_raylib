#include "resource_manager.h"
#include <iostream>
#include <filesystem>

ResourceManager::~ResourceManager() {
    unloadAll();
}

Texture2D* ResourceManager::loadTexture(const std::string& path, const std::string& name) {
    if (textures.find(name) != textures.end()) {
        std::cout << "[ResourceManager] Texture '" << name << "' already loaded." << std::endl;
        return &textures[name];
    }

    if (!std::filesystem::exists(path)) {
        std::cerr << "[ResourceManager] Error: Texture file not found: " << path << std::endl;
        return nullptr;
    }

    Texture2D texture = LoadTexture(path.c_str());
    if (texture.id == 0) {
        std::cerr << "[ResourceManager] Error: Failed to load texture: " << path << std::endl;
        return nullptr;
    }

    textures[name] = texture;
    std::cout << "[ResourceManager] Loaded texture '" << name << "' from: " << path << std::endl;
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
    std::cerr << "[ResourceManager] Warning: Texture '" << name << "' not found." << std::endl;
    return nullptr;
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
    std::cout << "[ResourceManager] Unloading all resources..." << std::endl;
    
    for (auto& [name, texture] : textures) {
        UnloadTexture(texture);
        std::cout << "[ResourceManager] Unloaded texture: " << name << std::endl;
    }
    textures.clear();

    for (auto& [name, sound] : sounds) {
        UnloadSound(sound);
        std::cout << "[ResourceManager] Unloaded sound: " << name << std::endl;
    }
    sounds.clear();
}

void ResourceManager::unloadTexture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        UnloadTexture(it->second);
        textures.erase(it);
        std::cout << "[ResourceManager] Unloaded texture: " << name << std::endl;
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