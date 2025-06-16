#include "render/sprite_batch.h"
#include <spdlog/spdlog.h>
#include <algorithm>

void SpriteBatch::begin() {
    if (isActive) {
        spdlog::warn("[SpriteBatch] begin() called while batch is already active");
        return;
    }
    
    isActive = true;
    sprites.clear();
    textureGroups.clear();
    drawCallCount = 0;
}

void SpriteBatch::end() {
    if (!isActive) {
        spdlog::warn("[SpriteBatch] end() called while batch is not active");
        return;
    }
    
    isActive = false;
    sprites.clear();
    textureGroups.clear();
    drawCallCount = 0;
}

void SpriteBatch::addSprite(Texture2D* texture, const Rectangle& sourceRect, 
                            const Vector2& position, const Color& tint) {
    if (!isActive) {
        spdlog::warn("[SpriteBatch] addSprite() called while batch is not active");
        return;
    }
    
    if (!texture) {
        spdlog::warn("[SpriteBatch] addSprite() called with null texture");
        return;
    }
    
    // Add sprite to the list
    sprites.push_back({texture, sourceRect, position, tint});
}

void SpriteBatch::flush() {
    if (!isActive) {
        spdlog::warn("[SpriteBatch] flush() called while batch is not active");
        return;
    }
    
    if (sprites.empty()) {
        return;
    }
    
    // Group sprites by texture
    textureGroups.clear();
    for (auto& sprite : sprites) {
        textureGroups[sprite.texture->id].push_back(&sprite);
    }
    
    // Draw each texture group
    drawCallCount = 0;
    for (const auto& [textureId, spriteGroup] : textureGroups) {
        if (!spriteGroup.empty()) {
            // In a real implementation, we would batch draw all sprites with the same texture
            // For now, we just count the draw calls
            drawCallCount++;
            
            // TODO: Implement actual batched rendering using rlgl
            // This would involve:
            // 1. Building vertex buffer with all sprite vertices
            // 2. Setting the texture once
            // 3. Drawing all sprites in one draw call
        }
    }
}