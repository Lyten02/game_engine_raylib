#pragma once

#include <vector>
#include <unordered_map>
#include "raylib.h"

// Represents a single sprite in the batch
struct BatchedSprite {
    Texture2D* texture;
    Rectangle sourceRect;
    Vector2 position;
    Color tint;
};

class SpriteBatch {
private:
    std::vector<BatchedSprite> sprites;
    std::unordered_map<unsigned int, std::vector<BatchedSprite*>> textureGroups;
    size_t drawCallCount = 0;
    bool isActive = false;
    
public:
    SpriteBatch() = default;
    ~SpriteBatch() = default;
    
    // Begin collecting sprites
    void begin();
    
    // End collecting and clear buffers
    void end();
    
    // Add a sprite to the batch
    void addSprite(Texture2D* texture, const Rectangle& sourceRect, 
                   const Vector2& position, const Color& tint);
    
    // Flush all sprites (draw them)
    void flush();
    
    // Get statistics
    size_t getDrawCallCount() const { return drawCallCount; }
    size_t getSpriteCount() const { return sprites.size(); }
};