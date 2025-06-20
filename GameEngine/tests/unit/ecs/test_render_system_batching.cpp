#include <iostream>
#include <vector>
#include "systems/render_system.h"
#include "render/sprite_batch.h"
#include "components/transform.h"
#include "components/sprite.h"
#include "resources/resource_manager.h"
#include <entt/entt.hpp>
#include "raylib.h"

bool testRenderSystemBatching() {
    std::cout << "Testing RenderSystem with sprite batching..." << std::endl;
    
    // Initialize minimal RayLib
    SetTraceLogLevel(LOG_NONE);
    InitWindow(100, 100, "Test");
    
    // Create components
    entt::registry registry;
    RenderSystem renderSystem;
    renderSystem.initialize();
    
    // Create resource manager
    ResourceManager resourceManager;
    resourceManager.setSilentMode(true);
    resourceManager.setHeadlessMode(false);
    resourceManager.setRayLibInitialized(true);
    
    // Create real test textures
    std::vector<Texture2D> textures;
    for (int i = 0; i < 5; i++) {
        Image img = GenImageColor(32, 32, (Color){(unsigned char)(i * 50), 0, 0, 255});
        Texture2D tex = LoadTextureFromImage(img);
        UnloadImage(img);
        textures.push_back(tex);
    }
    
    // Create many entities with sprites
    const int ENTITIES_PER_TEXTURE = 20;
    for (int t = 0; t < textures.size(); t++) {
        for (int i = 0; i < ENTITIES_PER_TEXTURE; i++) {
            auto entity = registry.create();
            
            auto& transform = registry.emplace<TransformComponent>(entity);
            transform.position = { (float)(i * 10), (float)(t * 50), 0 };
            
            auto& sprite = registry.emplace<Sprite>(entity);
            sprite.texture = &textures[t];
            sprite.sourceRect = { 0, 0, 32, 32 };
            sprite.tint = WHITE;
        }
    }
    
    // Set up camera
    Camera2D camera = { 0 };
    renderSystem.setCamera2D(camera);
    
    // Render
    BeginDrawing();
    ClearBackground(BLACK);
    renderSystem.update(registry);
    EndDrawing();
    
    // Get batch statistics
    auto* batch = renderSystem.getSpriteBatch();
    if (!batch) {
        std::cerr << "FAIL: SpriteBatch is null" << std::endl;
        CloseWindow();
        return false;
    }
    
    // Without batching: 100 sprites = 100 draw calls
    // With batching: 100 sprites with 5 textures = 5 draw calls
    size_t expectedDrawCalls = textures.size();
    size_t actualDrawCalls = batch->getLastFrameDrawCalls();
    
    // Clean up textures
    for (auto& tex : textures) {
        UnloadTexture(tex);
    }
    
    CloseWindow();
    
    if (actualDrawCalls != expectedDrawCalls) {
        std::cerr << "FAIL: Expected " << expectedDrawCalls << " draw calls, got " 
                  << actualDrawCalls << std::endl;
        return false;
    }
    
    std::cout << "PASS: " << (ENTITIES_PER_TEXTURE * textures.size()) 
              << " sprites rendered with only " << actualDrawCalls << " draw calls!" << std::endl;
    std::cout << "      Performance improvement: " << (100.0f / actualDrawCalls) << "x" << std::endl;
    
    return true;
}

int main() {
    std::cout << "Running RenderSystem batching test..." << std::endl;
    
    if (testRenderSystemBatching()) {
        std::cout << "\nTest passed!" << std::endl;
        return 0;
    } else {
        std::cerr << "\nTest failed!" << std::endl;
        return 1;
    }
}