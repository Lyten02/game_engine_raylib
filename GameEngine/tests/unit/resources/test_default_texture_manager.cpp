#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
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
    
    // Get default texture through missing texture request
    Texture2D* tex = rm.getTexture("missing_texture");
    
    TEST("Headless texture is not null", tex != nullptr);
    TEST("Headless texture has id=0", tex->id == 0);
    TEST("Headless texture has correct width", tex->width == 64);
    TEST("Headless texture has correct height", tex->height == 64);
    TEST("Headless texture has correct format", tex->format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
}

void testGraphicsMode() {
    std::cout << "\n=== Testing Graphics Mode ===" << std::endl;
    
    // Create resource manager without RayLib initialized
    ResourceManager rm;
    rm.setHeadlessMode(false);
    rm.setRayLibInitialized(false);
    rm.setSilentMode(true);
    
    // Get default texture - should use dummy texture when RayLib not initialized
    Texture2D* tex = rm.getTexture("missing_texture");
    
    TEST("Graphics mode without RayLib returns valid texture", tex != nullptr);
    TEST("Graphics mode without RayLib uses dummy texture", tex->id == 0);
    TEST("Graphics texture has correct width", tex->width == 64);
    TEST("Graphics texture has correct height", tex->height == 64);
}

void testMultiThreadedAccess() {
    std::cout << "\n=== Testing Multi-threaded Access ===" << std::endl;
    
    // Create a shared resource manager
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(true);
    
    // Access default texture from multiple threads
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);
    std::atomic<bool> allPointersEqual(true);
    Texture2D* firstPointer = nullptr;
    std::mutex pointerMutex;
    
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&rm, &successCount, &firstPointer, &allPointersEqual, &pointerMutex, i]() {
            try {
                // Small delay to increase chance of race conditions
                std::this_thread::sleep_for(std::chrono::milliseconds(i * 5));
                
                Texture2D* tex = rm.getTexture("missing_texture_" + std::to_string(i));
                if (tex && tex->width == 64 && tex->height == 64) {
                    successCount++;
                    
                    // Check if all threads get the same pointer
                    std::lock_guard<std::mutex> lock(pointerMutex);
                    if (firstPointer == nullptr) {
                        firstPointer = tex;
                    } else if (firstPointer != tex) {
                        allPointersEqual = false;
                    }
                }
            } catch (...) {
                // Count as failure if exception thrown
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    TEST("All threads accessed texture successfully", successCount == 10);
    TEST("All threads got the same default texture pointer", allPointersEqual.load());
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
        
        // Access default texture through missing texture
        rm.getTexture("missing");
    }
    
    TEST("No crashes after multiple create/destroy cycles", true);
    
    // Test that missing textures don't grow the map
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(true);
    
    size_t initialCount = rm.getLoadedTexturesCount();
    
    // Request many missing textures
    for (int i = 0; i < 1000; ++i) {
        rm.getTexture("missing_" + std::to_string(i));
    }
    
    size_t finalCount = rm.getLoadedTexturesCount();
    TEST("Missing textures don't grow the map", finalCount == initialCount);
}

void testConsistency() {
    std::cout << "\n=== Testing Default Texture Consistency ===" << std::endl;
    
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(true);
    
    // Get default texture through different missing names
    Texture2D* tex1 = rm.getTexture("missing1");
    Texture2D* tex2 = rm.getTexture("missing2");
    Texture2D* tex3 = rm.loadTexture("nonexistent.png", "test");
    
    TEST("All missing textures return same pointer", tex1 == tex2 && tex2 == tex3);
    TEST("Default texture is consistent", tex1 != nullptr);
}

int main() {
    std::cout << "=== Default Texture Manager Unit Tests ===" << std::endl;
    
    testHeadlessMode();
    testGraphicsMode();
    testMultiThreadedAccess();
    testMemoryLeaks();
    testConsistency();
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << testsPassed << std::endl;
    std::cout << "Failed: " << testsFailed << std::endl;
    
    return testsFailed == 0 ? 0 : 1;
}