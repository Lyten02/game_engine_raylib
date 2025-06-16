#pragma once

#include <vector>
#include <raylib.h>

struct SpriteData {
    Texture* texture;
    Rectangle srcRect;
    Vector2 position;
    Color tint;
};

class SpriteBatch {
public:
    void begin();
    void addSprite(Texture* texture, const Rectangle& srcRect, 
                  const Vector2& position, const Color& tint);
    void end();
    void render();
    
private:
    std::vector<SpriteData> sprites;
};