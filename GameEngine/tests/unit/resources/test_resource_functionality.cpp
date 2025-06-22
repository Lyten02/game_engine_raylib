#include <iostream>
#include <cassert>
#include "resources/resource_manager.h"

void test_texture_functionality() {
    std::cout << "Testing ResourceManager texture functionality...\n";
    
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(false);
    rm.setRayLibInitialized(false);
    
    // Test 1: Default texture is valid
    std::cout << "Test 1: Default texture validity...\n";
    Texture2D* tex1 = rm.getTexture("missing1");
    assert(tex1 != nullptr);
    assert(tex1->id == 0);  // In headless mode, should be dummy texture
    assert(tex1->width == 64);
    assert(tex1->height == 64);
    std::cout << "✅ Default texture is valid\n";
    
    // Test 2: Multiple requests return same pointer
    std::cout << "\nTest 2: Consistency of default texture...\n";
    Texture2D* tex2 = rm.getTexture("missing1");
    assert(tex1 == tex2);
    std::cout << "✅ Same missing name returns same pointer\n";
    
    // Test 3: Different missing names return same default texture
    std::cout << "\nTest 3: All missing textures use same default...\n";
    Texture2D* tex3 = rm.getTexture("missing2");
    assert(tex1 == tex3);
    std::cout << "✅ Different missing names return same default texture\n";
    
    // Test 4: Map remains empty for missing textures
    std::cout << "\nTest 4: Map doesn't grow with missing textures...\n";
    for (int i = 0; i < 100; i++) {
        std::string name = "missing_texture_" + std::to_string(i);
        rm.getTexture(name);
    }
    assert(rm.getLoadedTexturesCount() == 0);
    std::cout << "✅ Map size remains 0 after 100 missing texture requests\n";
    
    // Test 5: Load texture with dummy path in headless
    std::cout << "\nTest 5: Loading texture in headless mode...\n";
    Texture2D* tex4 = rm.loadTexture("dummy_path.png", "test_texture");
    assert(tex4 == rm.getTexture("missing"));  // Should return default texture
    assert(rm.getLoadedTexturesCount() == 0);  // Should not be stored
    std::cout << "✅ Loading texture in headless returns default without storing\n";
    
    std::cout << "\n✅ All texture functionality tests passed!\n";
}

void test_mode_switching() {
    std::cout << "\nTesting mode switching...\n";
    
    ResourceManager rm;
    
    // Start in headless
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    rm.setSilentMode(true);
    
    std::cout << "Test 1: Get texture in headless mode...\n";
    Texture2D* tex1 = rm.getTexture("test");
    assert(tex1 != nullptr);
    assert(tex1->id == 0);  // Dummy texture
    std::cout << "✅ Headless mode returns dummy texture\n";
    
    // Switch to normal mode (simulation)
    std::cout << "\nTest 2: Switch to normal mode...\n";
    rm.setHeadlessMode(false);
    rm.setRayLibInitialized(true);
    
    // Note: Without actual RayLib initialization, this will still return dummy
    Texture2D* tex2 = rm.getTexture("test");
    assert(tex2 != nullptr);
    std::cout << "✅ Mode switch handled gracefully\n";
    
    std::cout << "\n✅ Mode switching tests passed!\n";
}

void test_memory_efficiency() {
    std::cout << "\nTesting memory efficiency...\n";
    
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(true);
    
    // Test with large number of missing texture requests
    const int NUM_REQUESTS = 10000;
    
    std::cout << "Making " << NUM_REQUESTS << " missing texture requests...\n";
    for (int i = 0; i < NUM_REQUESTS; i++) {
        std::string name = "missing_" + std::to_string(i);
        Texture2D* tex = rm.getTexture(name);
        assert(tex != nullptr);
    }
    
    std::cout << "Map size after " << NUM_REQUESTS << " requests: " << rm.getLoadedTexturesCount() << "\n";
    assert(rm.getLoadedTexturesCount() == 0);
    std::cout << "✅ Memory efficient - no growth with missing textures\n";
}

int main() {
    std::cout << "=== ResourceManager Deep Functionality Tests ===\n\n";
    
    test_texture_functionality();
    test_mode_switching();
    test_memory_efficiency();
    
    std::cout << "\n🎉 All deep functionality tests passed!\n";
    return 0;
}