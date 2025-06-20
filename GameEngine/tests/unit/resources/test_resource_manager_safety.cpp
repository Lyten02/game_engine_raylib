#include <iostream>
#include <vector>
#include <thread>
#include "resources/resource_manager.h"

// Test that default texture is always valid
void testDefaultTextureAlwaysValid() {
    std::cout << "Testing default texture validity..." << std::endl;
    
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    // Request a non-existent texture
    Texture2D* tex1 = rm.getTexture("non_existent");
    if (!tex1) {
        std::cerr << "FAIL: getTexture returned nullptr!" << std::endl;
        exit(1);
    }
    
    // Request another non-existent texture
    Texture2D* tex2 = rm.getTexture("another_non_existent");
    if (!tex2) {
        std::cerr << "FAIL: getTexture returned nullptr on second call!" << std::endl;
        exit(1);
    }
    
    // Both should point to the same default texture
    if (tex1 != tex2) {
        std::cerr << "FAIL: Different default textures returned!" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Default texture is always valid" << std::endl;
}

// Test that no nullptr is ever returned
void testNoNullptrReturns() {
    std::cout << "Testing no nullptr returns..." << std::endl;
    
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    // Test various scenarios
    std::vector<std::string> testNames = {
        "test1", "test2", "test3", 
        "", " ", "!@#$%^&*()", 
        "very_long_name_that_exceeds_normal_length_expectations_and_might_cause_issues"
    };
    
    for (const auto& name : testNames) {
        Texture2D* tex = rm.getTexture(name);
        if (!tex) {
            std::cerr << "FAIL: getTexture(\"" << name << "\") returned nullptr!" << std::endl;
            exit(1);
        }
    }
    
    std::cout << "PASS: No nullptr returns" << std::endl;
}

// Test that missing textures return default
void testMissingTexturesReturnDefault() {
    std::cout << "Testing missing textures return default..." << std::endl;
    
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    // Load texture from non-existent file
    Texture2D* tex1 = rm.loadTexture("/non/existent/path.png", "missing");
    if (!tex1) {
        std::cerr << "FAIL: loadTexture with missing file returned nullptr!" << std::endl;
        exit(1);
    }
    
    // Get the same texture by name
    Texture2D* tex2 = rm.getTexture("missing");
    if (!tex2) {
        std::cerr << "FAIL: getTexture for missing file returned nullptr!" << std::endl;
        exit(1);
    }
    
    // In headless mode, missing textures should return default without being stored
    // So tex1 and tex2 should both be the default texture
    Texture2D* defaultTex = rm.getTexture("any_non_existent");
    if (tex1 != defaultTex || tex2 != defaultTex) {
        std::cerr << "FAIL: Missing textures not returning default!" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Missing textures return default" << std::endl;
}

// Test that unload doesn't break defaults
void testUnloadDoesNotBreakDefaults() {
    std::cout << "Testing unload doesn't break defaults..." << std::endl;
    
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    // Get default texture
    Texture2D* tex1 = rm.getTexture("test");
    if (!tex1) {
        std::cerr << "FAIL: Initial getTexture returned nullptr!" << std::endl;
        exit(1);
    }
    
    // Try to unload it (shouldn't affect default)
    rm.unloadTexture("test");
    
    // Get it again
    Texture2D* tex2 = rm.getTexture("test");
    if (!tex2) {
        std::cerr << "FAIL: getTexture after unload returned nullptr!" << std::endl;
        exit(1);
    }
    
    // Should still be the same default texture
    if (tex1 != tex2) {
        std::cerr << "FAIL: Default texture changed after unload!" << std::endl;
        exit(1);
    }
    
    // Unload all
    rm.unloadAll();
    
    // Should still work
    Texture2D* tex3 = rm.getTexture("test");
    if (!tex3) {
        std::cerr << "FAIL: getTexture after unloadAll returned nullptr!" << std::endl;
        exit(1);
    }
    
    if (tex1 != tex3) {
        std::cerr << "FAIL: Default texture changed after unloadAll!" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Unload doesn't break defaults" << std::endl;
}

// Test pointer validity across ResourceManager lifetime
void testPointerValidityAcrossLifetime() {
    std::cout << "Testing pointer validity across lifetime..." << std::endl;
    
    std::vector<Texture2D*> pointers;
    
    {
        ResourceManager rm;
        rm.setSilentMode(true);
        rm.setHeadlessMode(true);
        rm.setRayLibInitialized(false);
        
        // Collect some pointers
        for (int i = 0; i < 10; i++) {
            pointers.push_back(rm.getTexture("test" + std::to_string(i)));
        }
        
        // All should be valid
        for (auto ptr : pointers) {
            if (!ptr) {
                std::cerr << "FAIL: Got nullptr during collection!" << std::endl;
                exit(1);
            }
        }
    }
    
    // ResourceManager is destroyed, but static default texture should still be valid
    // We can check that all pointers point to the same address
    void* firstAddr = pointers[0];
    for (auto ptr : pointers) {
        if (ptr != firstAddr) {
            std::cerr << "FAIL: Not all pointers point to the same default texture!" << std::endl;
            exit(1);
        }
    }
    
    std::cout << "PASS: Pointer validity maintained across lifetime" << std::endl;
}

int main() {
    std::cout << "=== ResourceManager Safety Tests ===" << std::endl;
    
    testDefaultTextureAlwaysValid();
    testNoNullptrReturns();
    testMissingTexturesReturnDefault();
    testUnloadDoesNotBreakDefaults();
    testPointerValidityAcrossLifetime();
    
    std::cout << "\nAll safety tests passed!" << std::endl;
    return 0;
}