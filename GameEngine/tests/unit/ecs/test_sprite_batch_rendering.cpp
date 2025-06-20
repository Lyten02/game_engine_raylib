#include <iostream>
#include <vector>
#include "render/sprite_batch.h"
#include "components/sprite.h"
#include "components/transform.h"
#include "raylib.h"
#include "rlgl.h"

// Test that SpriteBatch can actually render sprites
bool testSpriteBatchRenderOutput() {
    std::cout << "Testing SpriteBatch render output..." << std::endl;
    
    // Initialize minimal RayLib context
    SetTraceLogLevel(LOG_NONE);
    InitWindow(100, 100, "Test");
    
    SpriteBatch batch;
    batch.begin();
    
    // Create a test texture
    Image img = GenImageColor(64, 64, RED);
    Texture2D texture = LoadTextureFromImage(img);
    UnloadImage(img);
    
    Rectangle sourceRect = { 0, 0, 64, 64 };
    Vector2 position = { 10, 10 };
    Color tint = WHITE;
    
    // Add sprite
    batch.addSprite(&texture, sourceRect, position, tint);
    
    // Clear screen
    BeginDrawing();
    ClearBackground(BLACK);
    
    // Render the batch
    batch.render();
    
    // Check if something was drawn (simplified check)
    // In a real test, we would read back the framebuffer and verify pixel colors
    bool renderSuccess = batch.getRenderedSpriteCount() == 1;
    
    EndDrawing();
    
    batch.end();
    UnloadTexture(texture);
    CloseWindow();
    
    if (!renderSuccess) {
        std::cerr << "FAIL: No sprites were rendered" << std::endl;
        return false;
    }
    
    std::cout << "PASS: Sprite rendered successfully" << std::endl;
    return true;
}

bool testSpriteBatchVertexGeneration() {
    std::cout << "Testing SpriteBatch vertex generation..." << std::endl;
    
    SpriteBatch batch;
    batch.begin();
    
    // Create test data
    Texture2D texture;
    texture.id = 1;
    texture.width = 64;
    texture.height = 64;
    
    Rectangle sourceRect = { 0, 0, 32, 32 }; // Half texture
    Vector2 position = { 100, 200 };
    Color tint = { 255, 128, 64, 255 };
    
    batch.addSprite(&texture, sourceRect, position, tint);
    
    // Get vertex data (we'll add a method for testing)
    auto vertices = batch.getVertexData();
    
    // Each sprite should generate 4 vertices
    if (vertices.size() != 4) {
        std::cerr << "FAIL: Expected 4 vertices, got " << vertices.size() << std::endl;
        return false;
    }
    
    // Check first vertex position (top-left)
    if (vertices[0].x != position.x || vertices[0].y != position.y) {
        std::cerr << "FAIL: First vertex position incorrect" << std::endl;
        return false;
    }
    
    // Check texture coordinates
    if (vertices[0].u != 0.0f || vertices[0].v != 0.0f) {
        std::cerr << "FAIL: First vertex UV incorrect" << std::endl;
        return false;
    }
    
    // Check color
    if (vertices[0].color.r != tint.r || vertices[0].color.g != tint.g ||
        vertices[0].color.b != tint.b || vertices[0].color.a != tint.a) {
        std::cerr << "FAIL: Vertex color incorrect" << std::endl;
        return false;
    }
    
    batch.end();
    
    std::cout << "PASS: Vertex generation correct" << std::endl;
    return true;
}

bool testSpriteBatchBatchedDrawCalls() {
    std::cout << "Testing batched draw calls..." << std::endl;
    
    SetTraceLogLevel(LOG_NONE);
    InitWindow(100, 100, "Test");
    
    SpriteBatch batch;
    batch.begin();
    
    // Create multiple textures
    std::vector<Texture2D> textures;
    for (int i = 0; i < 3; i++) {
        Image img = GenImageColor(32, 32, (Color){(unsigned char)(i * 80), 0, 0, 255});
        textures.push_back(LoadTextureFromImage(img));
        UnloadImage(img);
    }
    
    Rectangle sourceRect = { 0, 0, 32, 32 };
    Color tint = WHITE;
    
    // Add sprites with same texture together
    for (int i = 0; i < 10; i++) {
        Vector2 pos = { (float)(i * 10), 0 };
        batch.addSprite(&textures[0], sourceRect, pos, tint);
    }
    
    for (int i = 0; i < 10; i++) {
        Vector2 pos = { (float)(i * 10), 40 };
        batch.addSprite(&textures[1], sourceRect, pos, tint);
    }
    
    BeginDrawing();
    ClearBackground(BLACK);
    
    // Render the batch
    batch.render();
    
    // Verify we had 2 draw calls (one per texture)
    if (batch.getActualDrawCallCount() != 2) {
        std::cerr << "FAIL: Expected 2 draw calls, got " << batch.getActualDrawCallCount() << std::endl;
        EndDrawing();
        batch.end();
        for (auto& tex : textures) UnloadTexture(tex);
        CloseWindow();
        return false;
    }
    
    EndDrawing();
    batch.end();
    
    for (auto& tex : textures) UnloadTexture(tex);
    CloseWindow();
    
    std::cout << "PASS: Batched rendering with correct draw call count" << std::endl;
    return true;
}

int main() {
    std::cout << "Running SpriteBatch rendering tests..." << std::endl;
    
    bool allTestsPassed = true;
    
    // Test 1: Vertex generation
    if (!testSpriteBatchVertexGeneration()) {
        allTestsPassed = false;
    }
    
    // Test 2: Render output
    if (!testSpriteBatchRenderOutput()) {
        allTestsPassed = false;
    }
    
    // Test 3: Batched draw calls
    if (!testSpriteBatchBatchedDrawCalls()) {
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