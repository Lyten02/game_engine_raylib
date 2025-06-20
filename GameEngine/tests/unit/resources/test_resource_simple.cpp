#include <iostream>
#include "resources/resource_manager.h"

int main() {
    // Test in headless mode
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    std::cout << "Initial texture count: " << rm.getLoadedTexturesCount() << std::endl;
    
    // Test requesting non-existent textures
    for (int i = 0; i < 10; i++) {
        std::string name = "missing_" + std::to_string(i);
        Texture2D* tex = rm.getTexture(name);
        if (!tex) {
            std::cerr << "ERROR: getTexture returned nullptr for " << name << std::endl;
            return 1;
        }
    }
    
    std::cout << "After 10 missing requests: " << rm.getLoadedTexturesCount() << std::endl;
    
    // Should still be 0 - map shouldn't grow
    if (rm.getLoadedTexturesCount() != 0) {
        std::cerr << "ERROR: Map grew with missing textures! Count: " << rm.getLoadedTexturesCount() << std::endl;
        return 1;
    }
    
    // Test loading with non-existent path
    Texture2D* tex1 = rm.loadTexture("/fake/path.png", "test1");
    if (!tex1) {
        std::cerr << "ERROR: loadTexture returned nullptr" << std::endl;
        return 1;
    }
    
    std::cout << "After loading missing file: " << rm.getLoadedTexturesCount() << std::endl;
    
    // Should still be 0
    if (rm.getLoadedTexturesCount() != 0) {
        std::cerr << "ERROR: Map grew when loading missing file! Count: " << rm.getLoadedTexturesCount() << std::endl;
        return 1;
    }
    
    // Test that we get the same default texture
    Texture2D* tex2 = rm.getTexture("test1");
    if (tex1 != tex2) {
        std::cerr << "ERROR: Different pointers for same missing texture" << std::endl;
        return 1;
    }
    
    std::cout << "SUCCESS: All tests passed!" << std::endl;
    std::cout << "Map size remained at: " << rm.getLoadedTexturesCount() << std::endl;
    
    return 0;
}