#include <iostream>
#include "src/resources/resource_manager.h"
#include "raylib.h"

int main() {
    // Initialize Raylib window
    InitWindow(800, 600, "Default Texture Test");
    SetTargetFPS(60);
    
    // Create resource manager
    ResourceManager resourceManager;
    
    // Try to load non-existent texture
    std::cout << "\nTesting missing texture loading:\n";
    Texture2D* texture1 = resourceManager.loadTexture("nonexistent.png", "missing1");
    if (texture1) {
        std::cout << "✓ loadTexture returned a texture (not nullptr)\n";
        std::cout << "  Texture ID: " << texture1->id << "\n";
        std::cout << "  Texture size: " << texture1->width << "x" << texture1->height << "\n";
    } else {
        std::cout << "✗ loadTexture returned nullptr\n";
    }
    
    // Try to get non-existent texture
    std::cout << "\nTesting getTexture with missing texture:\n";
    Texture2D* texture2 = resourceManager.getTexture("notloaded");
    if (texture2) {
        std::cout << "✓ getTexture returned a texture (not nullptr)\n";
        std::cout << "  Texture ID: " << texture2->id << "\n";
    } else {
        std::cout << "✗ getTexture returned nullptr\n";
    }
    
    // Test loading real texture if available
    std::cout << "\nTesting real texture loading:\n";
    Texture2D* texture3 = resourceManager.loadTexture("assets/textures/test_sprite.png", "real");
    if (texture3) {
        std::cout << "✓ Real texture loaded successfully\n";
        std::cout << "  Texture ID: " << texture3->id << "\n";
        std::cout << "  Texture size: " << texture3->width << "x" << texture3->height << "\n";
    }
    
    // Draw test
    std::cout << "\nRendering test - press ESC to exit...\n";
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(DARKGRAY);
        
        // Draw default texture
        if (texture1) {
            DrawTexture(*texture1, 100, 100, WHITE);
            DrawText("Missing Texture", 100, 180, 20, WHITE);
        }
        
        // Draw real texture if loaded
        if (texture3 && texture3->id != texture1->id) {
            DrawTexture(*texture3, 300, 100, WHITE);
            DrawText("Real Texture", 300, 180, 20, WHITE);
        }
        
        DrawText("Default Texture Test - Pink/Black checkerboard should be visible", 10, 10, 20, WHITE);
        DrawFPS(10, 40);
        
        EndDrawing();
    }
    
    CloseWindow();
    
    std::cout << "\nTest completed successfully!\n";
    return 0;
}