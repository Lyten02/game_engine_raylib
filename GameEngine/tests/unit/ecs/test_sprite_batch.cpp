#include <iostream>
#include <vector>
#include "../src/render/sprite_batch.h"
#include "../src/components/sprite.h"
#include "../src/components/transform.h"
#include "raylib.h"

// Test basic SpriteBatch functionality
bool testSpriteBatchCreation() {
    std::cout << "Testing SpriteBatch creation..." << std::endl;
    
    SpriteBatch batch;
    
    // Test initial state
    if (batch.getDrawCallCount() != 0) {
        std::cerr << "FAIL: Initial draw call count should be 0" << std::endl;
        return false;
    }
    
    if (batch.getSpriteCount() != 0) {
        std::cerr << "FAIL: Initial sprite count should be 0" << std::endl;
        return false;
    }
    
    std::cout << "PASS: SpriteBatch created successfully" << std::endl;
    return true;
}

bool testSpriteBatchBeginEnd() {
    std::cout << "Testing SpriteBatch begin/end..." << std::endl;
    
    SpriteBatch batch;
    
    // Test begin
    batch.begin();
    
    // Should be able to add sprites after begin
    // Test end - should clear the batch
    batch.end();
    
    if (batch.getSpriteCount() != 0) {
        std::cerr << "FAIL: Sprite count should be 0 after end()" << std::endl;
        return false;
    }
    
    std::cout << "PASS: Begin/end works correctly" << std::endl;
    return true;
}

bool testSpriteBatchAddSprite() {
    std::cout << "Testing SpriteBatch sprite addition..." << std::endl;
    
    SpriteBatch batch;
    batch.begin();
    
    // Create test data
    Texture2D texture;
    texture.id = 1;
    texture.width = 64;
    texture.height = 64;
    
    Rectangle sourceRect = { 0, 0, 64, 64 };
    Vector2 position = { 100, 100 };
    Color tint = WHITE;
    
    // Add sprite
    batch.addSprite(&texture, sourceRect, position, tint);
    
    if (batch.getSpriteCount() != 1) {
        std::cerr << "FAIL: Sprite count should be 1 after adding sprite" << std::endl;
        return false;
    }
    
    // Add more sprites
    for (int i = 0; i < 10; i++) {
        position.x = i * 70.0f;
        batch.addSprite(&texture, sourceRect, position, tint);
    }
    
    if (batch.getSpriteCount() != 11) {
        std::cerr << "FAIL: Sprite count should be 11, got " << batch.getSpriteCount() << std::endl;
        return false;
    }
    
    batch.end();
    
    std::cout << "PASS: Sprite addition works correctly" << std::endl;
    return true;
}

bool testSpriteBatchTextureGrouping() {
    std::cout << "Testing SpriteBatch texture grouping..." << std::endl;
    
    SpriteBatch batch;
    batch.begin();
    
    // Create different textures
    Texture2D texture1, texture2, texture3;
    texture1.id = 1;
    texture2.id = 2;
    texture3.id = 3;
    
    Rectangle sourceRect = { 0, 0, 64, 64 };
    Vector2 position = { 0, 0 };
    Color tint = WHITE;
    
    // Add sprites with different textures
    batch.addSprite(&texture1, sourceRect, position, tint);
    batch.addSprite(&texture2, sourceRect, position, tint);
    batch.addSprite(&texture1, sourceRect, position, tint); // Same as first
    batch.addSprite(&texture3, sourceRect, position, tint);
    batch.addSprite(&texture2, sourceRect, position, tint); // Same as second
    
    // After flush, should have 3 draw calls (one per unique texture)
    batch.flush();
    
    if (batch.getDrawCallCount() != 3) {
        std::cerr << "FAIL: Expected 3 draw calls for 3 unique textures, got " 
                  << batch.getDrawCallCount() << std::endl;
        return false;
    }
    
    batch.end();
    
    std::cout << "PASS: Texture grouping works correctly" << std::endl;
    return true;
}

bool testSpriteBatchPerformance() {
    std::cout << "Testing SpriteBatch performance with many sprites..." << std::endl;
    
    SpriteBatch batch;
    batch.begin();
    
    // Create test textures
    const int NUM_TEXTURES = 10;
    const int SPRITES_PER_TEXTURE = 100;
    std::vector<Texture2D> textures(NUM_TEXTURES);
    
    for (int i = 0; i < NUM_TEXTURES; i++) {
        textures[i].id = i + 1;
        textures[i].width = 64;
        textures[i].height = 64;
    }
    
    Rectangle sourceRect = { 0, 0, 64, 64 };
    Color tint = WHITE;
    
    // Add many sprites with different textures
    for (int i = 0; i < SPRITES_PER_TEXTURE; i++) {
        for (int j = 0; j < NUM_TEXTURES; j++) {
            Vector2 position = { (float)(i * 10), (float)(j * 10) };
            batch.addSprite(&textures[j], sourceRect, position, tint);
        }
    }
    
    size_t totalSprites = batch.getSpriteCount();
    if (totalSprites != NUM_TEXTURES * SPRITES_PER_TEXTURE) {
        std::cerr << "FAIL: Expected " << (NUM_TEXTURES * SPRITES_PER_TEXTURE) 
                  << " sprites, got " << totalSprites << std::endl;
        return false;
    }
    
    // Flush and check draw calls
    batch.flush();
    
    // Should have one draw call per texture
    if (batch.getDrawCallCount() != NUM_TEXTURES) {
        std::cerr << "FAIL: Expected " << NUM_TEXTURES << " draw calls, got " 
                  << batch.getDrawCallCount() << std::endl;
        return false;
    }
    
    std::cout << "PASS: " << totalSprites << " sprites rendered with only " 
              << batch.getDrawCallCount() << " draw calls!" << std::endl;
    std::cout << "      (Without batching: " << totalSprites << " draw calls)" << std::endl;
    
    batch.end();
    return true;
}

int main() {
    std::cout << "Running SpriteBatch tests..." << std::endl;
    
    bool allTestsPassed = true;
    
    // Test 1: Creation
    if (!testSpriteBatchCreation()) {
        allTestsPassed = false;
    }
    
    // Test 2: Begin/End
    if (!testSpriteBatchBeginEnd()) {
        allTestsPassed = false;
    }
    
    // Test 3: Add sprite
    if (!testSpriteBatchAddSprite()) {
        allTestsPassed = false;
    }
    
    // Test 4: Texture grouping
    if (!testSpriteBatchTextureGrouping()) {
        allTestsPassed = false;
    }
    
    // Test 5: Performance
    if (!testSpriteBatchPerformance()) {
        allTestsPassed = false;
    }
    
    if (allTestsPassed) {
        std::cout << "\nAll tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << "\nSome tests failed!" << std::endl;
        return 1;
    }
}