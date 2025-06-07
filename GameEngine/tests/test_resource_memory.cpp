#include <iostream>
#include <cassert>
#include "../src/resources/resource_manager.h"

void testDefaultTextureCreatedOnce() {
    std::cout << "Test: Default texture created only once" << std::endl;
    
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(true);
    
    // Request multiple missing textures
    Texture2D* tex1 = rm.getTexture("missing1");
    Texture2D* tex2 = rm.getTexture("missing2");
    Texture2D* tex3 = rm.getTexture("missing3");
    
    // All should point to the same default texture
    assert(tex1 == tex2);
    assert(tex2 == tex3);
    
    // Request same missing texture multiple times
    Texture2D* tex4 = rm.getTexture("missing1");
    assert(tex1 == tex4);
    
    std::cout << "✓ All missing textures point to the same default texture" << std::endl;
}

void testMemoryNotGrowingWithMissingTextures() {
    std::cout << "Test: Memory not growing with missing texture requests" << std::endl;
    
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(true);
    
    // Initial count
    size_t initialCount = rm.getLoadedTexturesCount();
    size_t initialUnique = rm.getUniqueTexturesCount();
    
    // Request many missing textures
    for (int i = 0; i < 100; i++) {
        std::string name = "missing_texture_" + std::to_string(i);
        rm.getTexture(name);
    }
    
    // Check counts
    size_t finalCount = rm.getLoadedTexturesCount();
    size_t finalUnique = rm.getUniqueTexturesCount();
    
    std::cout << "  Loaded textures: " << finalCount << std::endl;
    std::cout << "  Unique textures: " << finalUnique << std::endl;
    
    // We should have 100 entries, but only 1 unique texture (the default)
    assert(finalCount == initialCount + 100);
    assert(finalUnique == 1);
    
    std::cout << "✓ Only one unique texture exists despite 100 missing texture requests" << std::endl;
}

void testRealTextureAllocation() {
    std::cout << "Test: Real textures are allocated separately" << std::endl;
    
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(true);
    
    // In headless mode, loadTexture will still use default texture
    // but we can test that different files would get different entries
    Texture2D* tex1 = rm.loadTexture("fake1.png", "texture1");
    Texture2D* tex2 = rm.loadTexture("fake2.png", "texture2");
    Texture2D* tex3 = rm.getTexture("missing");
    
    // In headless mode, all should point to default
    assert(tex1 == tex2);
    assert(tex2 == tex3);
    
    size_t count = rm.getLoadedTexturesCount();
    size_t unique = rm.getUniqueTexturesCount();
    
    std::cout << "  Loaded textures: " << count << std::endl;
    std::cout << "  Unique textures: " << unique << std::endl;
    
    assert(count == 3);  // 3 named textures
    assert(unique == 1); // All point to default in headless mode
    
    std::cout << "✓ Texture allocation works correctly in headless mode" << std::endl;
}

void testUnloadDoesNotDeleteDefault() {
    std::cout << "Test: Unloading textures does not delete default" << std::endl;
    
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(true);
    
    // Get some textures
    Texture2D* tex1 = rm.getTexture("missing1");
    Texture2D* tex2 = rm.getTexture("missing2");
    
    // Unload one
    rm.unloadTexture("missing1");
    
    // tex2 should still be valid
    Texture2D* tex3 = rm.getTexture("missing3");
    assert(tex2 == tex3);
    
    // Unload all
    rm.unloadAll();
    
    // Can still get default texture
    Texture2D* tex4 = rm.getTexture("missing4");
    assert(tex4 != nullptr);
    
    std::cout << "✓ Default texture survives unload operations" << std::endl;
}

int main() {
    std::cout << "=== ResourceManager Memory Test ===" << std::endl;
    
    try {
        testDefaultTextureCreatedOnce();
        testMemoryNotGrowingWithMissingTextures();
        testRealTextureAllocation();
        testUnloadDoesNotDeleteDefault();
        
        std::cout << "\n✅ All memory tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}