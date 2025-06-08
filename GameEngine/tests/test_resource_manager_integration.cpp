#include "../src/resources/resource_manager.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <spdlog/spdlog.h>

// Integration test for ResourceManager
// Tests complete workflow: headless mode, graphics mode, exception recovery
// Verifies all code paths work correctly together

bool testHeadlessMode() {
    std::cout << "\n=== Testing Headless Mode ===" << std::endl;
    
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(false);
    
    // Test 1: Default texture in headless mode
    auto& defaultTex = rm.getDefaultTexture();
    if (defaultTex.id != 0 || defaultTex.width != 64 || defaultTex.height != 64) {
        std::cerr << "✗ Headless default texture has incorrect properties" << std::endl;
        return false;
    }
    std::cout << "✓ Default texture created correctly in headless mode" << std::endl;
    
    // Test 2: Loading texture in headless mode (should return default)
    auto* tex = rm.loadTexture("../assets/textures/test_sprite.png", "test_texture");
    if (tex != &defaultTex) {
        std::cerr << "✗ Headless mode should return default texture for all loads" << std::endl;
        return false;
    }
    std::cout << "✓ Texture loading returns default texture in headless mode" << std::endl;
    
    // Test 3: Multiple textures in headless mode
    for (int i = 0; i < 5; ++i) {
        std::string name = "headless_tex_" + std::to_string(i);
        auto* t = rm.loadTexture("path/to/texture.png", name);
        if (t != &defaultTex) {
            std::cerr << "✗ All textures should be default in headless mode" << std::endl;
            return false;
        }
    }
    std::cout << "✓ Multiple texture loads handled correctly in headless mode" << std::endl;
    
    return true;
}

bool testGraphicsMode() {
    std::cout << "\n=== Testing Graphics Mode ===" << std::endl;
    
    ResourceManager rm;
    rm.setHeadlessMode(false);
    rm.setSilentMode(false);
    rm.setRayLibInitialized(true);  // Simulate RayLib being initialized
    
    // Test 1: Default texture in graphics mode
    auto& defaultTex = rm.getDefaultTexture();
    if (defaultTex.width != 64 || defaultTex.height != 64) {
        std::cerr << "✗ Graphics mode default texture has incorrect dimensions" << std::endl;
        return false;
    }
    std::cout << "✓ Default texture created correctly in graphics mode" << std::endl;
    
    // Test 2: Loading non-existent texture (should return default)
    auto* missingTex = rm.loadTexture("non_existent_file.png", "missing");
    if (missingTex != &defaultTex) {
        std::cerr << "✗ Missing texture should return default texture" << std::endl;
        return false;
    }
    std::cout << "✓ Missing texture returns default texture correctly" << std::endl;
    
    // Test 3: Getting non-existent texture
    auto* getTex = rm.getTexture("never_loaded");
    if (getTex != &defaultTex) {
        std::cerr << "✗ Getting non-existent texture should return default" << std::endl;
        return false;
    }
    std::cout << "✓ Getting non-existent texture returns default correctly" << std::endl;
    
    return true;
}

bool testModeTransitions() {
    std::cout << "\n=== Testing Mode Transitions ===" << std::endl;
    
    // Test 1: Start in headless, transition to graphics
    {
        ResourceManager rm;
        rm.setHeadlessMode(true);
        rm.setSilentMode(true);
        
        // Get default texture in headless mode
        auto& headlessTex = rm.getDefaultTexture();
        if (headlessTex.id != 0) {
            std::cerr << "✗ Headless texture should have ID 0" << std::endl;
            return false;
        }
        
        // Transition to graphics mode
        rm.setHeadlessMode(false);
        rm.setRayLibInitialized(true);
        
        // Default texture should still be the same (created once)
        auto& graphicsTex = rm.getDefaultTexture();
        if (&headlessTex != &graphicsTex) {
            std::cerr << "✗ Default texture should not change after mode transition" << std::endl;
            return false;
        }
        std::cout << "✓ Mode transition preserves default texture" << std::endl;
    }
    
    // Test 2: RayLib not initialized in graphics mode
    {
        ResourceManager rm;
        rm.setHeadlessMode(false);
        rm.setSilentMode(true);
        rm.setRayLibInitialized(false);  // RayLib not ready
        
        auto& tex = rm.getDefaultTexture();
        if (tex.id != 0) {
            std::cerr << "✗ Should create dummy texture when RayLib not initialized" << std::endl;
            return false;
        }
        std::cout << "✓ Handles RayLib not initialized correctly" << std::endl;
    }
    
    return true;
}

bool testConcurrentWorkflow() {
    std::cout << "\n=== Testing Concurrent Workflow ===" << std::endl;
    
    ResourceManager rm;
    rm.setHeadlessMode(false);
    rm.setSilentMode(true);
    rm.setRayLibInitialized(true);
    
    std::atomic<int> successCount{0};
    std::atomic<int> errorCount{0};
    
    // Simulate a real workflow with multiple threads
    auto worker = [&rm, &successCount, &errorCount](int workerId) {
        try {
            // Each worker does a series of operations
            for (int i = 0; i < 10; ++i) {
                // Load unique texture
                std::string uniqueName = "worker_" + std::to_string(workerId) + "_tex_" + std::to_string(i);
                auto* tex1 = rm.loadTexture("../assets/textures/test_sprite.png", uniqueName);
                
                // Load shared texture
                std::string sharedName = "shared_texture_" + std::to_string(i % 3);
                auto* tex2 = rm.loadTexture("../assets/textures/test_sprite.png", sharedName);
                
                // Get default texture
                auto& defaultTex = rm.getDefaultTexture();
                
                // Get existing texture
                auto* tex3 = rm.getTexture(sharedName);
                
                if (tex1 && tex2 && tex3 && defaultTex.width == 64) {
                    successCount++;
                } else {
                    errorCount++;
                }
                
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
            
            // Cleanup some textures
            for (int i = 0; i < 5; ++i) {
                std::string name = "worker_" + std::to_string(workerId) + "_tex_" + std::to_string(i);
                rm.unloadTexture(name);
            }
            
        } catch (const std::exception& e) {
            spdlog::error("Worker {} caught exception: {}", workerId, e.what());
            errorCount += 10;
        }
    };
    
    // Launch workers
    std::vector<std::thread> threads;
    const int numWorkers = 8;
    
    auto startTime = std::chrono::steady_clock::now();
    
    for (int i = 0; i < numWorkers; ++i) {
        threads.emplace_back(worker, i);
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Concurrent workflow completed in " << duration.count() << " ms" << std::endl;
    std::cout << "Success: " << successCount.load() << ", Errors: " << errorCount.load() << std::endl;
    
    if (errorCount.load() > 0) {
        std::cerr << "✗ Concurrent workflow had errors" << std::endl;
        return false;
    }
    std::cout << "✓ Concurrent workflow completed successfully" << std::endl;
    
    // Verify resource cleanup
    size_t remainingTextures = rm.getUniqueTexturesCount();
    std::cout << "Remaining textures after partial cleanup: " << remainingTextures << std::endl;
    
    rm.unloadAll();
    if (rm.getUniqueTexturesCount() != 0) {
        std::cerr << "✗ UnloadAll did not clean up all textures" << std::endl;
        return false;
    }
    std::cout << "✓ Resource cleanup working correctly" << std::endl;
    
    return true;
}

bool testErrorRecovery() {
    std::cout << "\n=== Testing Error Recovery ===" << std::endl;
    
    // Test 1: Multiple ResourceManager instances
    std::vector<std::unique_ptr<ResourceManager>> managers;
    for (int i = 0; i < 5; ++i) {
        auto rm = std::make_unique<ResourceManager>();
        rm->setHeadlessMode(true);
        rm->setSilentMode(true);
        
        try {
            auto& tex = rm->getDefaultTexture();
            if (tex.width != 64) {
                std::cerr << "✗ Instance " << i << " has incorrect default texture" << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "✗ Instance " << i << " threw exception: " << e.what() << std::endl;
            return false;
        }
        
        managers.push_back(std::move(rm));
    }
    std::cout << "✓ Multiple instances handled correctly" << std::endl;
    
    // Test 2: Rapid creation/destruction
    for (int i = 0; i < 10; ++i) {
        ResourceManager rm;
        rm.setHeadlessMode(true);
        rm.setSilentMode(true);
        
        auto& tex = rm.getDefaultTexture();
        rm.loadTexture("test.png", "rapid_test");
        // Destructor will be called automatically
    }
    std::cout << "✓ Rapid creation/destruction handled correctly" << std::endl;
    
    return true;
}

int main() {
    spdlog::set_level(spdlog::level::info);
    std::cout << "=== ResourceManager Integration Test ===" << std::endl;
    
    bool allTestsPassed = true;
    
    // Run all test suites
    allTestsPassed &= testHeadlessMode();
    allTestsPassed &= testGraphicsMode();
    allTestsPassed &= testModeTransitions();
    allTestsPassed &= testConcurrentWorkflow();
    allTestsPassed &= testErrorRecovery();
    
    if (allTestsPassed) {
        std::cout << "\n✅ All integration tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ Some integration tests failed!" << std::endl;
        return 1;
    }
}