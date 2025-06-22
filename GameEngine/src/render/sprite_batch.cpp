#include "render/sprite_batch.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include "rlgl.h"

void SpriteBatch::begin() {
    if (isActive) {
        spdlog::warn("[SpriteBatch] begin() called while batch is already active");
        return;
    }

    isActive = true;
    sprites.clear();
    textureGroups.clear();
    vertexBuffer.clear();
    drawCallCount = 0;
    actualDrawCallCount = 0;
    renderedSpriteCount = 0;
}

void SpriteBatch::end() {
    if (!isActive) {
        spdlog::warn("[SpriteBatch] end() called while batch is not active");
        return;
    }

    // Save last frame statistics before clearing
    lastFrameDrawCalls = actualDrawCallCount;
    lastFrameSprites = renderedSpriteCount;

    isActive = false;
    sprites.clear();
    textureGroups.clear();
    vertexBuffer.clear();
    drawCallCount = 0;
    actualDrawCallCount = 0;
    renderedSpriteCount = 0;
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

void SpriteBatch::render() {
    if (!isActive) {
        spdlog::warn("[SpriteBatch] render() called while batch is not active");
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

    actualDrawCallCount = 0;
    renderedSpriteCount = 0;

    // Render each texture group
    for (const auto& [textureId, spriteGroup] : textureGroups) {
        if (spriteGroup.empty()) continue;

        // Build vertex buffer for this texture group
        vertexBuffer.clear();
        vertexBuffer.reserve(spriteGroup.size() * 4); // 4 vertices per sprite

        for (const auto* sprite : spriteGroup) {
            buildSpriteVertices(*sprite, vertexBuffer);
        }

        // Get the texture from first sprite
        Texture2D* texture = spriteGroup[0]->texture;

        // Begin custom rendering
        rlSetTexture(texture->id);
        rlBegin(RL_QUADS);

        // Submit all vertices
        for (const auto& vertex : vertexBuffer) {
            rlColor4ub(vertex.color.r, vertex.color.g, vertex.color.b, vertex.color.a);
            rlTexCoord2f(vertex.u, vertex.v);
            rlVertex3f(vertex.x, vertex.y, vertex.z);
        }

        rlEnd();

        actualDrawCallCount++;
        renderedSpriteCount += spriteGroup.size();
    }
}

void SpriteBatch::buildSpriteVertices(const BatchedSprite& sprite, std::vector<SpriteVertex>& vertices) {
    // Calculate sprite dimensions
    float width = sprite.sourceRect.width;
    float height = sprite.sourceRect.height;

    // Calculate texture coordinates
    float u1 = sprite.sourceRect.x / sprite.texture->width;
    float v1 = sprite.sourceRect.y / sprite.texture->height;
    float u2 = (sprite.sourceRect.x + sprite.sourceRect.width) / sprite.texture->width;
    float v2 = (sprite.sourceRect.y + sprite.sourceRect.height) / sprite.texture->height;

    // Top-left vertex
    vertices.push_back( {
        sprite.position.x, sprite.position.y, 0.0f,
        u1, v1,
        sprite.tint
    });

    // Top-right vertex
    vertices.push_back( {
        sprite.position.x + width, sprite.position.y, 0.0f,
        u2, v1,
        sprite.tint
    });

    // Bottom-right vertex
    vertices.push_back( {
        sprite.position.x + width, sprite.position.y + height, 0.0f,
        u2, v2,
        sprite.tint
    });

    // Bottom-left vertex
    vertices.push_back( {
        sprite.position.x, sprite.position.y + height, 0.0f,
        u1, v2,
        sprite.tint
    });
}

std::vector<SpriteVertex> SpriteBatch::getVertexData() const {
    std::vector<SpriteVertex> result;
    if (!sprites.empty()) {
        result.reserve(sprites.size() * 4);
        for (const auto& sprite : sprites) {
            const_cast<SpriteBatch*>(this)->buildSpriteVertices(sprite, result);
        }
    }
    return result;
}
