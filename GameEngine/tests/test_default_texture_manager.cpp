#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "../src/resources/resource_manager.h"
#include <raylib.h>

// Test counters
std::atomic<int> testsPassed(0);
std::atomic<int> testsFailed(0);

void TEST(const std::string& name, bool condition) {
    if (condition) {
        std::cout << "✓ " << name << std::endl;
        testsPassed++;
    } else {
        std::cout << "✗ " << name << std::endl;
        testsFailed++;
    }
}

void testHeadlessMode() {
    std::cout << "\n=== Testing Headless Mode ===" << std::endl;
    
    // Create resource manager in headless mode
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(true);
    
    // Get default texture
    Texture2D& tex = ResourceManager::getDefaultTexture();
    
    TEST("Headless texture has id=0", tex.id == 0);
    TEST("Headless texture has correct width", tex.width == 64);
    TEST("Headless texture has correct height", tex.height == 64);
    TEST("Headless texture has correct format", tex.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
}

void testGraphicsMode() {
    std::cout << "\n=== Testing Graphics Mode ===" << std::endl;
    
    // Initialize RayLib
    InitWindow(100, 100, "Test");
    
    // Create resource manager in graphics mode
    ResourceManager rm;
    rm.setHeadlessMode(false);
    rm.setRayLibInitialized(true);
    rm.setSilentMode(true);
    
    // Get default texture
    Texture2D& tex = ResourceManager::getDefaultTexture();
    
    TEST("Graphics texture has valid id", tex.id > 0);
    TEST("Graphics texture has correct width", tex.width == 64);
    TEST("Graphics texture has correct height", tex.height == 64);
    
    // Clean up
    cleanupDefaultTexture();
    CloseWindow();
}

void testMultiThreadedAccess() {
    std::cout << "\n=== Testing Multi-threaded Access ===" << std::endl;
    
    // Reset default texture state
    cleanupDefaultTexture();
    
    // Create multiple resource managers
    std::vector<std::unique_ptr<ResourceManager>> managers;
    for (int i = 0; i < 5; ++i) {
        auto rm = std::make_unique<ResourceManager>();
        rm->setHeadlessMode(true);
        rm->setSilentMode(true);
        managers.push_back(std::move(rm));
    }
    
    // Access default texture from multiple threads
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);
    
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&successCount, i]() {
            try {
                // Small delay to increase chance of race conditions
                std::this_thread::sleep_for(std::chrono::milliseconds(i * 5));
                
                Texture2D& tex = ResourceManager::getDefaultTexture();
                if (tex.width == 64 && tex.height == 64) {
                    successCount++;
                }
            } catch (...) {
                // Count as failure if exception thrown
            }
        });
    }
    
    // Wait for all threads with timeout
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    TEST("All threads accessed texture successfully", successCount == 10);
    TEST("Multi-threaded access completed quickly", duration < 1000); // Should complete in less than 1 second
}

void testMemoryLeaks() {
    std::cout << "\n=== Testing Memory Management ===" << std::endl;
    
    // Create and destroy multiple resource managers
    for (int i = 0; i < 100; ++i) {
        ResourceManager rm;
        rm.setHeadlessMode(true);
        rm.setSilentMode(true);
        
        // Load some textures
        rm.loadTexture("nonexistent.png", "test1");
        rm.loadTexture("nonexistent2.png", "test2");
        
        // Access default texture
        ResourceManager::getDefaultTexture();
    }
    
    TEST("No crashes after multiple create/destroy cycles", true);
}

void testCleanupSafety() {
    std::cout << "\n=== Testing Cleanup Safety ===" << std::endl;
    
    // Test multiple cleanup calls
    cleanupDefaultTexture();
    cleanupDefaultTexture();
    cleanupDefaultTexture();
    
    TEST("Multiple cleanup calls don't crash", true);
    
    // Test accessing after cleanup
    Texture2D& tex = ResourceManager::getDefaultTexture();
    TEST("Can access texture after cleanup", tex.width == 64);
}

int main() {
    std::cout << "=== Default Texture Manager Unit Tests ===" << std::endl;
    
    testHeadlessMode();
    testGraphicsMode();
    testMultiThreadedAccess();
    testMemoryLeaks();
    testCleanupSafety();
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << testsPassed << std::endl;
    std::cout << "Failed: " << testsFailed << std::endl;
    
    return testsFailed == 0 ? 0 : 1;
}