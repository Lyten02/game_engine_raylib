// Test to verify that all missing textures return the same pointer
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "resources/resource_manager.h"

int main() {
    std::cout << "=== Testing ResourceManager pointer consistency ===" << std::endl;
    
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    // Test 1: Multiple missing textures should return the same pointer
    std::cout << "Test 1: Missing texture pointer consistency..." << std::endl;
    
    Texture2D* tex1 = rm.getTexture("missing1");
    Texture2D* tex2 = rm.getTexture("missing2");
    Texture2D* tex3 = rm.getTexture("completely_different_name");
    
    assert(tex1 != nullptr);
    assert(tex2 != nullptr);
    assert(tex3 != nullptr);
    
    if (tex1 != tex2 || tex2 != tex3) {
        std::cerr << "FAIL: Different missing textures returned different pointers!" << std::endl;
        std::cerr << "  tex1: " << tex1 << std::endl;
        std::cerr << "  tex2: " << tex2 << std::endl;
        std::cerr << "  tex3: " << tex3 << std::endl;
        return 1;
    }
    std::cout << "  ✓ All missing textures return same pointer: " << tex1 << std::endl;
    
    // Test 2: Map should remain empty
    std::cout << "Test 2: Map size check..." << std::endl;
    if (rm.getLoadedTexturesCount() != 0) {
        std::cerr << "FAIL: Map size is " << rm.getLoadedTexturesCount() << ", expected 0" << std::endl;
        return 1;
    }
    std::cout << "  ✓ Map remains empty (size: 0)" << std::endl;
    
    // Test 3: loadTexture with missing file
    std::cout << "Test 3: loadTexture with missing file..." << std::endl;
    Texture2D* tex4 = rm.loadTexture("/nonexistent/path.png", "test_load");
    assert(tex4 != nullptr);
    
    if (tex4 != tex1) {
        std::cerr << "FAIL: loadTexture returned different pointer for missing file!" << std::endl;
        std::cerr << "  loadTexture result: " << tex4 << std::endl;
        std::cerr << "  getTexture result:  " << tex1 << std::endl;
        return 1;
    }
    std::cout << "  ✓ loadTexture returns same default pointer" << std::endl;
    
    // Test 4: Map still empty after loadTexture
    if (rm.getLoadedTexturesCount() != 0) {
        std::cerr << "FAIL: Map grew after loadTexture! Size: " << rm.getLoadedTexturesCount() << std::endl;
        return 1;
    }
    std::cout << "  ✓ Map still empty after loadTexture" << std::endl;
    
    // Test 5: Repeated requests for same missing texture
    std::cout << "Test 5: Repeated requests consistency..." << std::endl;
    for (int i = 0; i < 100; i++) {
        Texture2D* tex = rm.getTexture("repeated_missing");
        if (tex != tex1) {
            std::cerr << "FAIL: Repeated request " << i << " returned different pointer!" << std::endl;
            return 1;
        }
    }
    std::cout << "  ✓ 100 repeated requests returned same pointer" << std::endl;
    
    // Test 6: Many different missing textures
    std::cout << "Test 6: Many different missing textures..." << std::endl;
    for (int i = 0; i < 1000; i++) {
        std::string name = "missing_texture_" + std::to_string(i);
        Texture2D* tex = rm.getTexture(name);
        if (tex != tex1) {
            std::cerr << "FAIL: Missing texture #" << i << " returned different pointer!" << std::endl;
            return 1;
        }
    }
    
    if (rm.getLoadedTexturesCount() != 0) {
        std::cerr << "FAIL: Map grew with 1000 missing textures! Size: " << rm.getLoadedTexturesCount() << std::endl;
        return 1;
    }
    std::cout << "  ✓ 1000 different missing textures: same pointer, map size 0" << std::endl;
    
    std::cout << "\n=== ALL TESTS PASSED! ===" << std::endl;
    std::cout << "Memory leak is fixed: missing textures don't grow the map" << std::endl;
    return 0;
}