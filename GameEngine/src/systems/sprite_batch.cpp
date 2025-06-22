#include "sprite_batch.h"
#include <algorithm>
#include <raylib.h>

void SpriteBatch::begin() {
    sprites.clear();
}

void SpriteBatch::addSprite(Texture* texture, const Rectangle& srcRect, 
                           const Vector2& position, const Color& tint) {
    sprites.push_back({texture, srcRect, position, tint});
}

void SpriteBatch::end() {
    // Sort sprites by texture to minimize state changes
    std::sort(sprites.begin(), sprites.end(), 
        [](const SpriteData& a, const SpriteData& b) {
            return a.texture < b.texture;
        });
}

void SpriteBatch::render() {
    for (const auto& sprite : sprites) {
        if (sprite.texture) {
            DrawTexturePro(*sprite.texture, 
                          sprite.srcRect,
                          {sprite.position.x, sprite.position.y, 
                           sprite.srcRect.width, sprite.srcRect.height},
                          {0, 0}, 0.0f, sprite.tint);
        }
    }
}

void SpriteBatch::flush() {
    // Render all sprites and clear the batch
    render();
    sprites.clear();
}

const std::vector<SpriteData>& SpriteBatch::getVertexData() const {
    // Return sprite data for testing/diagnostic purposes
    return sprites;
}