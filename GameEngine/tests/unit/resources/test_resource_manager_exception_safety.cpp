#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include "../src/resources/resource_manager.h"

int main() {
    std::cout << "=== ResourceManager Exception Safety Test (Minimal) ===" << std::endl;
    
    // Test 1: Basic exception safety
    {
        std::cout << "\nTest 1: Basic exception safety..." << std::endl;
        ResourceManager rm;
        rm.setHeadlessMode(true);
        rm.setSilentMode(true);
        
        // Test with uninitialized RayLib
        rm.setRayLibInitialized(false);
        Texture2D& tex1 = rm.getDefaultTexture();
        if (tex1.width == 64 && tex1.height == 64) {
            std::cout << "✓ Fallback texture created successfully" << std::endl;
        } else {
            std::cout << "✗ Failed to create fallback texture" << std::endl;
            return 1;
        }
    }
    
    // Test 2: Concurrent exception safety
    {
        std::cout << "\nTest 2: Concurrent exception safety..." << std::endl;
        ResourceManager rm;
        rm.setHeadlessMode(false);
        rm.setSilentMode(true);
        rm.setRayLibInitialized(false);
        
        std::atomic<int> successCount{0};
        std::vector<std::thread> threads;
        
        for (int i = 0; i < 20; ++i) {
            threads.emplace_back([&rm, &successCount]() {
                try {
                    Texture2D& tex = rm.getDefaultTexture();
                    if (tex.width == 64 && tex.height == 64) {
                        successCount++;
                    }
                } catch (...) {
                    // Should not throw
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        if (successCount == 20) {
            std::cout << "✓ All threads handled exceptions safely" << std::endl;
        } else {
            std::cout << "✗ Some threads failed: " << successCount << "/20" << std::endl;
            return 1;
        }
    }
    
    // Test 3: Mode switching exception safety
    {
        std::cout << "\nTest 3: Mode switching exception safety..." << std::endl;
        ResourceManager rm;
        rm.setSilentMode(true);
        
        // Switch modes and verify texture remains valid
        rm.setHeadlessMode(true);
        rm.setRayLibInitialized(false);
        Texture2D& tex1 = rm.getDefaultTexture();
        
        rm.setHeadlessMode(false);
        rm.setRayLibInitialized(true);
        Texture2D& tex2 = rm.getDefaultTexture();
        
        if (&tex1 == &tex2 && tex1.width == 64) {
            std::cout << "✓ Texture remains consistent across mode switches" << std::endl;
        } else {
            std::cout << "✗ Texture changed during mode switch" << std::endl;
            return 1;
        }
    }
    
    // Test 4: Non-existent resource handling
    {
        std::cout << "\nTest 4: Non-existent resource handling..." << std::endl;
        ResourceManager rm;
        rm.setHeadlessMode(true);
        rm.setSilentMode(true);
        
        // Get non-existent texture - should return default
        Texture2D* tex = rm.getTexture("does_not_exist");
        Texture2D& defaultTex = rm.getDefaultTexture();
        
        if (tex == &defaultTex) {
            std::cout << "✓ Non-existent texture returns default" << std::endl;
        } else {
            std::cout << "✗ Non-existent texture didn't return default" << std::endl;
            return 1;
        }
    }
    
    std::cout << "\n✅ All exception safety tests passed!" << std::endl;
    return 0;
}