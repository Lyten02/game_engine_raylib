#include <iostream>
#include "../src/resources/resource_manager.h"
#include "raylib.h"

int main() {
    // Initialize RayLib in headless mode
    SetTraceLogLevel(LOG_NONE);
    ResourceManager resourceManager;
    resourceManager.setSilentMode(true);
    resourceManager.setHeadlessMode(true);
    resourceManager.setRayLibInitialized(false);
    
    std::cout << "Running ResourceManager memory test..." << std::endl;
    std::cout << "Initial texture count: " << resourceManager.getLoadedTexturesCount() << std::endl;
    
    // Test 1: Request 100 different missing textures
    for (int i = 0; i < 100; i++) {
        std::string name = "missing_texture_" + std::to_string(i);
        Texture2D* tex = resourceManager.getTexture(name);
        if (!tex) {
            std::cerr << "ERROR: getTexture returned nullptr" << std::endl;
            return 1;
        }
    }
    
    std::cout << "After 100 missing texture requests: " << resourceManager.getLoadedTexturesCount() << " textures in map" << std::endl;
    
    // Test 2: The map should NOT grow with missing textures
    if (resourceManager.getLoadedTexturesCount() > 0) {
        std::cerr << "FAIL: Map grew with missing textures! Count: " << resourceManager.getLoadedTexturesCount() << std::endl;
        return 1;
    }
    
    std::cout << "PASS: Map did not grow with missing texture requests" << std::endl;
    
    // Test 3: Load a texture with non-existent path
    Texture2D* tex = resourceManager.loadTexture("/non/existent/path.png", "test_missing");
    if (!tex) {
        std::cerr << "ERROR: loadTexture returned nullptr" << std::endl;
        return 1;
    }
    
    std::cout << "After loading missing file: " << resourceManager.getLoadedTexturesCount() << " textures in map" << std::endl;
    
    // Test 4: Map still shouldn't grow
    if (resourceManager.getLoadedTexturesCount() > 0) {
        std::cerr << "FAIL: Map grew when loading missing file!" << std::endl;
        return 1;
    }
    
    std::cout << "PASS: Map did not grow when loading missing file" << std::endl;
    
    // Test 5: Verify we get the same default texture pointer
    Texture2D* tex1 = resourceManager.getTexture("missing1");
    Texture2D* tex2 = resourceManager.getTexture("missing2");
    if (tex1 != tex2) {
        std::cerr << "FAIL: Different pointers for default texture!" << std::endl;
        return 1;
    }
    
    std::cout << "PASS: Same default texture pointer returned for all missing textures" << std::endl;
    
    std::cout << "\nAll tests passed! ResourceManager memory efficiency is working correctly." << std::endl;
    return 0;
}