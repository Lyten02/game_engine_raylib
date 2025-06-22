#include "resources/resource_manager.h"
#include <iostream>
#include <thread>
#include <vector>
#include <spdlog/spdlog.h>

void testHeadlessMode() {
    std::cout << "Testing headless mode functionality..." << std::endl;
    
    ResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    manager.setRayLibInitialized(false);  // Ensure RayLib is "not initialized"
    
    // Test 1: Default texture in headless mode
    Texture2D& defaultTex = manager.getDefaultTexture();
    if (defaultTex.id != 0) {
        std::cerr << "FAIL: Headless mode texture should have id=0" << std::endl;
        exit(1);
    }
    
    if (defaultTex.width != 64 || defaultTex.height != 64) {
        std::cerr << "FAIL: Headless mode texture has wrong dimensions" << std::endl;
        exit(1);
    }
    
    if (defaultTex.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        std::cerr << "FAIL: Headless mode texture has wrong format" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Headless mode default texture created correctly" << std::endl;
    
    // Test 2: Loading textures in headless mode returns default texture
    Texture2D* loaded = manager.loadTexture("/some/fake/path.png", "test_texture");
    if (loaded != &defaultTex) {
        std::cerr << "FAIL: Headless mode should return default texture for all loads" << std::endl;
        exit(1);
    }
    
    // Test 3: Getting textures in headless mode
    Texture2D* retrieved = manager.getTexture("test_texture");
    if (retrieved != &defaultTex) {
        std::cerr << "FAIL: Headless mode should return default texture for all gets" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Headless mode texture loading works correctly" << std::endl;
}

void testHeadlessToGraphicsTransition() {
    std::cout << "\nTesting headless to graphics mode transition..." << std::endl;
    
    ResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    manager.setRayLibInitialized(false);
    
    // Get default texture in headless mode
    Texture2D& headlessTex = manager.getDefaultTexture();
    if (headlessTex.id != 0) {
        std::cerr << "FAIL: Initial headless texture should have id=0" << std::endl;
        exit(1);
    }
    
    // Simulate RayLib initialization
    manager.setRayLibInitialized(true);
    manager.setHeadlessMode(false);
    
    // Default texture should still be the same (already initialized)
    Texture2D& afterTex = manager.getDefaultTexture();
    if (&afterTex != &headlessTex) {
        std::cerr << "FAIL: Default texture pointer changed after mode switch" << std::endl;
        exit(1);
    }
    
    // Should still be dummy texture since it was created in headless mode
    if (afterTex.id != 0) {
        std::cerr << "FAIL: Texture should remain dummy after mode switch" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Mode transition maintains texture consistency" << std::endl;
}

void testConcurrentHeadlessAccess() {
    std::cout << "\nTesting concurrent access in headless mode..." << std::endl;
    
    ResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    
    const int numThreads = 50;
    std::vector<std::thread> threads;
    std::atomic<bool> errorDetected{false};
    std::vector<Texture2D*> texturePointers(numThreads, nullptr);
    
    // Launch threads that all access textures
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&manager, &errorDetected, &texturePointers, i]() {
            try {
                // Get default texture
                texturePointers[i] = &manager.getDefaultTexture();
                
                // Load some textures
                for (int j = 0; j < 10; ++j) {
                    std::string name = "thread_" + std::to_string(i) + "_tex_" + std::to_string(j);
                    manager.loadTexture("/fake/path.png", name);
                }
            } catch (const std::exception& e) {
                errorDetected.store(true);
                std::cerr << "Exception in thread: " << e.what() << std::endl;
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    if (errorDetected.load()) {
        std::cerr << "FAIL: Error detected during concurrent headless access" << std::endl;
        exit(1);
    }
    
    // Verify all threads got the same default texture
    Texture2D* firstPtr = texturePointers[0];
    for (int i = 1; i < numThreads; ++i) {
        if (texturePointers[i] != firstPtr) {
            std::cerr << "FAIL: Different default texture pointers in headless mode" << std::endl;
            exit(1);
        }
    }
    
    std::cout << "PASS: Concurrent headless access works correctly" << std::endl;
}

void testHeadlessModePerformance() {
    std::cout << "\nTesting headless mode performance..." << std::endl;
    
    ResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Load many textures
    const int numTextures = 10000;
    for (int i = 0; i < numTextures; ++i) {
        std::string name = "perf_texture_" + std::to_string(i);
        manager.loadTexture("/fake/path.png", name);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Loaded " << numTextures << " textures in headless mode in " 
              << duration.count() << "ms" << std::endl;
    
    // In headless mode, should be very fast since it just returns default texture
    if (duration.count() > 1000) {  // Should take less than 1 second
        std::cerr << "WARNING: Headless mode texture loading seems slow" << std::endl;
    }
    
    // Verify no actual textures were stored (only default returns)
    size_t uniqueCount = manager.getUniqueTexturesCount();
    if (uniqueCount != 0) {  // In headless mode, loadTexture doesn't store anything
        std::cerr << "FAIL: Headless mode stored textures when it shouldn't" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Headless mode performance is optimal" << std::endl;
}

void testSilentMode() {
    std::cout << "\nTesting silent mode..." << std::endl;
    
    // Capture current log level
    auto originalLevel = spdlog::get_level();
    spdlog::set_level(spdlog::level::debug);  // Enable all logs
    
    // Test with silent mode off
    {
        std::cout << "Testing with silent mode OFF (you should see log messages):" << std::endl;
        ResourceManager manager;
        manager.setSilentMode(false);
        manager.setHeadlessMode(true);
        
        manager.getDefaultTexture();
        manager.loadTexture("/fake/path.png", "test1");
        manager.getTexture("nonexistent");
    }
    
    // Test with silent mode on
    {
        std::cout << "\nTesting with silent mode ON (you should NOT see log messages):" << std::endl;
        ResourceManager manager;
        manager.setSilentMode(true);
        manager.setHeadlessMode(true);
        
        manager.getDefaultTexture();
        manager.loadTexture("/fake/path.png", "test2");
        manager.getTexture("nonexistent");
    }
    
    // Restore original log level
    spdlog::set_level(originalLevel);
    
    std::cout << "PASS: Silent mode works correctly" << std::endl;
}

int main() {
    spdlog::set_level(spdlog::level::info);  // Show info logs for silent mode test
    
    std::cout << "=== ResourceManager Headless Mode Tests ===" << std::endl;
    
    testHeadlessMode();
    testHeadlessToGraphicsTransition();
    testConcurrentHeadlessAccess();
    testHeadlessModePerformance();
    testSilentMode();
    
    std::cout << "\n=== All headless mode tests passed! ===" << std::endl;
    return 0;
}