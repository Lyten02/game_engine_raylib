#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include "raylib.h"

class ResourceManager {
private:
    std::unordered_map<std::string, Texture2D> textures;
    std::unordered_map<std::string, Sound> sounds;
    Texture2D defaultTexture;
    bool silentMode = false;
    
    void createDefaultTexture();

public:
    ResourceManager();
    ~ResourceManager();
    
    void setSilentMode(bool silent) { silentMode = silent; }

    Texture2D* loadTexture(const std::string& path, const std::string& name);
    Sound* loadSound(const std::string& path, const std::string& name);
    
    Texture2D* getTexture(const std::string& name);
    Sound* getSound(const std::string& name);
    
    void unloadAll();
    void unloadTexture(const std::string& name);
    void unloadSound(const std::string& name);
};