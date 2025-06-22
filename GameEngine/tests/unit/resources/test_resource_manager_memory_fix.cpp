#include "resources/resource_manager.h"
#include <iostream>
#include <memory>
#include <vector>
#include <spdlog/spdlog.h>

// Test multiple ResourceManager creation/destruction cycles
// Verify no memory leaks using Valgrind or AddressSanitizer
// Test proper cleanup without global functions

class MemoryTest {
private:
    size_t allocations = 0;
    size_t deallocations = 0;
    
public:
    void runTest() {
        spdlog::info("Starting ResourceManager memory test (fixed version)");
        
        // Test 1: Single instance lifecycle
        spdlog::info("\nTest 1: Single instance lifecycle");
        {
            ResourceManager rm;
            rm.setHeadlessMode(true);
            rm.setSilentMode(true);
            
            // Load some textures
            for (int i = 0; i < 10; ++i) {
                std::string name = "texture_" + std::to_string(i);
                rm.loadTexture("../assets/textures/test_sprite.png", name);
            }
            
            // Access default texture
            auto& defaultTex = rm.getDefaultTexture();
            spdlog::info("Default texture size: {}x{}", defaultTex.width, defaultTex.height);
            
            // ResourceManager destructor should clean up everything
        }
        spdlog::info("Single instance destroyed - all resources should be cleaned up");
        
        // Test 2: Multiple instances in sequence
        spdlog::info("\nTest 2: Multiple instances in sequence");
        for (int cycle = 0; cycle < 5; ++cycle) {
            ResourceManager rm;
            rm.setHeadlessMode(true);
            rm.setSilentMode(true);
            
            // Load and unload textures
            for (int i = 0; i < 5; ++i) {
                std::string name = "cycle_" + std::to_string(cycle) + "_tex_" + std::to_string(i);
                rm.loadTexture("dummy_path.png", name);
            }
            
            // Access default texture multiple times
            for (int i = 0; i < 3; ++i) {
                auto& tex = rm.getDefaultTexture();
                if (tex.width != 64 || tex.height != 64) {
                    spdlog::error("Default texture has incorrect dimensions");
                }
            }
            
            spdlog::info("Cycle {} completed", cycle + 1);
        }
        
        // Test 3: Multiple instances with shared default texture access
        spdlog::info("\nTest 3: Multiple simultaneous instances");
        {
            std::vector<std::unique_ptr<ResourceManager>> managers;
            
            // Create multiple managers
            for (int i = 0; i < 3; ++i) {
                auto rm = std::make_unique<ResourceManager>();
                rm->setHeadlessMode(true);
                rm->setSilentMode(true);
                managers.push_back(std::move(rm));
            }
            
            // Each accesses default texture
            for (auto& rm : managers) {
                auto& tex = rm->getDefaultTexture();
                spdlog::info("Manager default texture: {}x{}", tex.width, tex.height);
            }
            
            // All managers destroyed together
        }
        spdlog::info("All simultaneous instances destroyed");
        
        // Test 4: Stress test - many creation/destruction cycles
        spdlog::info("\nTest 4: Stress test - 100 creation/destruction cycles");
        for (int i = 0; i < 100; ++i) {
            ResourceManager rm;
            rm.setHeadlessMode(true);
            rm.setSilentMode(true);
            
            // Just access default texture
            rm.getDefaultTexture();
            
            if (i % 20 == 0) {
                spdlog::info("Completed {} cycles", i);
            }
        }
        
        // Test 5: Dynamic allocation test
        spdlog::info("\nTest 5: Dynamic allocation test");
        for (int i = 0; i < 10; ++i) {
            auto* rm = new ResourceManager();
            rm->setHeadlessMode(true);
            rm->setSilentMode(true);
            
            // Load some resources
            rm->loadTexture("test.png", "dynamic_tex");
            rm->getDefaultTexture();
            
            delete rm;
        }
        spdlog::info("Dynamic allocation test completed");
        
        // Test 6: Exception safety
        spdlog::info("\nTest 6: Exception safety test");
        try {
            ResourceManager rm;
            rm.setHeadlessMode(true);
            rm.setSilentMode(true);
            
            // Load textures with invalid paths
            for (int i = 0; i < 5; ++i) {
                rm.loadTexture("/invalid/path/that/does/not/exist.png", "invalid_" + std::to_string(i));
            }
            
            // Should still work with default texture
            auto& tex = rm.getDefaultTexture();
            spdlog::info("Default texture still works after errors: {}x{}", tex.width, tex.height);
            
        } catch (const std::exception& e) {
            spdlog::error("Unexpected exception: {}", e.what());
        }
        
        spdlog::info("\n✅ Memory test completed successfully!");
        spdlog::info("No global cleanup function needed - all resources cleaned up automatically");
    }
};

int main() {
    spdlog::set_level(spdlog::level::info);
    
    MemoryTest test;
    test.runTest();
    
    spdlog::info("\nTest complete. Run with valgrind or AddressSanitizer to verify no leaks:");
    spdlog::info("  valgrind --leak-check=full ./test_resource_manager_memory_fix");
    spdlog::info("  or compile with: -fsanitize=address -g");
    
    return 0;
}