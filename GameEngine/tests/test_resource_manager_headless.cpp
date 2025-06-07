#include <iostream>
#include <cstring>
#include "../src/resources/resource_manager.h"

// Test that headless mode doesn't segfault
void testHeadlessModeNoSegfaults() {
    std::cout << "Testing headless mode no segfaults..." << std::endl;
    
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    // Try various operations that could cause segfaults
    for (int i = 0; i < 100; i++) {
        std::string name = "test_" + std::to_string(i);
        
        // Load texture
        Texture2D* tex1 = rm.loadTexture("/fake/path.png", name);
        if (!tex1) {
            std::cerr << "FAIL: loadTexture returned nullptr in headless mode!" << std::endl;
            exit(1);
        }
        
        // Get texture
        Texture2D* tex2 = rm.getTexture(name);
        if (!tex2) {
            std::cerr << "FAIL: getTexture returned nullptr in headless mode!" << std::endl;
            exit(1);
        }
        
        // Access texture fields (this would segfault if pointer was invalid)
        int width = tex1->width;
        int height = tex1->height;
        int id = tex1->id;
        int format = tex1->format;
        int mipmaps = tex1->mipmaps;
        
        // Verify reasonable values
        if (width <= 0 || height <= 0) {
            std::cerr << "FAIL: Invalid texture dimensions in headless mode!" << std::endl;
            exit(1);
        }
    }
    
    // Unload operations
    rm.unloadTexture("test_0");
    rm.unloadAll();
    
    // Should still work after unload
    Texture2D* tex = rm.getTexture("new_test");
    if (!tex) {
        std::cerr << "FAIL: getTexture after unloadAll returned nullptr!" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Headless mode operations complete without segfaults" << std::endl;
}

// Test headless default texture properties
void testHeadlessDefaultTexture() {
    std::cout << "Testing headless default texture properties..." << std::endl;
    
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    Texture2D* tex = rm.getTexture("test");
    if (!tex) {
        std::cerr << "FAIL: getTexture returned nullptr!" << std::endl;
        exit(1);
    }
    
    // Check texture properties
    if (tex->id != 0) {
        std::cerr << "FAIL: Headless texture should have id=0, got " << tex->id << std::endl;
        exit(1);
    }
    
    if (tex->width != 64 || tex->height != 64) {
        std::cerr << "FAIL: Expected 64x64 texture, got " << tex->width << "x" << tex->height << std::endl;
        exit(1);
    }
    
    if (tex->mipmaps != 1) {
        std::cerr << "FAIL: Expected mipmaps=1, got " << tex->mipmaps << std::endl;
        exit(1);
    }
    
    if (tex->format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        std::cerr << "FAIL: Expected PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, got " << tex->format << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Headless default texture has correct properties" << std::endl;
}

// Test headless load texture behavior
void testHeadlessLoadTexture() {
    std::cout << "Testing headless load texture behavior..." << std::endl;
    
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    // In headless mode, all textures should return the default texture
    Texture2D* tex1 = rm.loadTexture("/path/to/texture1.png", "tex1");
    Texture2D* tex2 = rm.loadTexture("/path/to/texture2.png", "tex2");
    Texture2D* tex3 = rm.loadTexture("/different/path.jpg", "tex3");
    
    if (!tex1 || !tex2 || !tex3) {
        std::cerr << "FAIL: loadTexture returned nullptr!" << std::endl;
        exit(1);
    }
    
    // All should be the same default texture
    if (tex1 != tex2 || tex2 != tex3) {
        std::cerr << "FAIL: Different textures returned in headless mode!" << std::endl;
        exit(1);
    }
    
    // Getting by name should also return the default
    Texture2D* get1 = rm.getTexture("tex1");
    Texture2D* get2 = rm.getTexture("tex2");
    
    if (get1 != tex1 || get2 != tex2) {
        std::cerr << "FAIL: getTexture doesn't return default in headless mode!" << std::endl;
        exit(1);
    }
    
    // Check that textures aren't actually stored
    if (rm.getLoadedTexturesCount() != 0) {
        std::cerr << "FAIL: Textures are being stored in headless mode!" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Headless load texture returns default without storing" << std::endl;
}

// Test transition between headless and normal modes
void testModeTransitions() {
    std::cout << "Testing mode transitions..." << std::endl;
    
    // Start in headless mode
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    Texture2D* tex1 = rm.getTexture("test1");
    if (!tex1) {
        std::cerr << "FAIL: getTexture in headless mode returned nullptr!" << std::endl;
        exit(1);
    }
    
    // Transition to "normal" mode (still without actual RayLib)
    rm.setHeadlessMode(false);
    rm.setRayLibInitialized(true);
    
    Texture2D* tex2 = rm.getTexture("test2");
    if (!tex2) {
        std::cerr << "FAIL: getTexture after mode change returned nullptr!" << std::endl;
        exit(1);
    }
    
    // Should still use the same default texture since it was already initialized
    if (tex1 != tex2) {
        std::cerr << "FAIL: Different default textures after mode change!" << std::endl;
        exit(1);
    }
    
    // Go back to headless
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    Texture2D* tex3 = rm.getTexture("test3");
    if (!tex3 || tex3 != tex1) {
        std::cerr << "FAIL: Default texture changed after returning to headless!" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Mode transitions maintain texture consistency" << std::endl;
}

// Test ResourceManager destruction in headless mode
void testHeadlessDestruction() {
    std::cout << "Testing headless ResourceManager destruction..." << std::endl;
    
    // Create and destroy multiple instances
    for (int i = 0; i < 10; i++) {
        ResourceManager rm;
        rm.setSilentMode(true);
        rm.setHeadlessMode(true);
        rm.setRayLibInitialized(false);
        
        // Use the resource manager
        for (int j = 0; j < 100; j++) {
            Texture2D* tex = rm.getTexture("test_" + std::to_string(j));
            if (!tex) {
                std::cerr << "FAIL: getTexture returned nullptr!" << std::endl;
                exit(1);
            }
        }
        
        rm.unloadAll();
    } // rm destroyed here
    
    std::cout << "PASS: Multiple ResourceManager instances created and destroyed safely" << std::endl;
}

int main() {
    std::cout << "=== ResourceManager Headless Tests ===" << std::endl;
    
    testHeadlessModeNoSegfaults();
    testHeadlessDefaultTexture();
    testHeadlessLoadTexture();
    testModeTransitions();
    testHeadlessDestruction();
    
    std::cout << "\nAll headless tests passed!" << std::endl;
    return 0;
}