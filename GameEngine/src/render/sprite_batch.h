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

// Vertex structure for sprite rendering
struct SpriteVertex {
    float x, y, z;      // Position
    float u, v;         // Texture coordinates
    Color color;        // Vertex color
};

class SpriteBatch {
private:
    std::vector<BatchedSprite> sprites;
    std::unordered_map<unsigned int, std::vector<BatchedSprite*>> textureGroups;
    std::vector<SpriteVertex> vertexBuffer;
    size_t drawCallCount = 0;
    size_t actualDrawCallCount = 0;
    size_t renderedSpriteCount = 0;
    size_t lastFrameDrawCalls = 0;
    size_t lastFrameSprites = 0;
    bool isActive = false;
    
    // Build vertices for a sprite
    void buildSpriteVertices(const BatchedSprite& sprite, std::vector<SpriteVertex>& vertices);
    
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
    
    // Flush all sprites (draw them) - for compatibility
    void flush();
    
    // Render all sprites using batched draw calls
    void render();
    
    // Get statistics
    size_t getDrawCallCount() const { return drawCallCount; }
    size_t getActualDrawCallCount() const { return actualDrawCallCount; }
    size_t getSpriteCount() const { return sprites.size(); }
    size_t getRenderedSpriteCount() const { return renderedSpriteCount; }
    
    // Get last frame statistics (persists after end())
    size_t getLastFrameDrawCalls() const { return lastFrameDrawCalls; }
    size_t getLastFrameSprites() const { return lastFrameSprites; }
    
    // For testing
    std::vector<SpriteVertex> getVertexData() const;
};