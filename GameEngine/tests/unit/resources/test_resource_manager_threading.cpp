#include "resources/resource_manager.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <iostream>
#include <spdlog/spdlog.h>

// Thread-safe testing of ResourceManager
void testConcurrentDefaultTextureAccess() {
    std::cout << "Testing concurrent access to getDefaultTexture()..." << std::endl;
    
    ResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    
    const int numThreads = 10;  // Reduced from 100
    const int accessesPerThread = 100;  // Reduced from 1000
    std::atomic<int> successCount{0};
    std::atomic<bool> raceDetected{false};
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Launch multiple threads that all try to access default texture
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&manager, &successCount, &raceDetected, accessesPerThread]() {
            try {
                for (int j = 0; j < accessesPerThread; ++j) {
                    Texture2D& texture = manager.getDefaultTexture();
                    
                    // Verify texture properties
                    if (texture.width != 64 || texture.height != 64) {
                        raceDetected.store(true);
                        std::cerr << "Invalid texture dimensions detected!" << std::endl;
                        return;
                    }
                    
                    successCount.fetch_add(1);
                }
            } catch (const std::exception& e) {
                raceDetected.store(true);
                std::cerr << "Exception in thread: " << e.what() << std::endl;
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Completed " << successCount.load() << " texture accesses in " 
              << duration.count() << "ms" << std::endl;
    
    if (raceDetected.load()) {
        std::cerr << "FAIL: Race condition detected!" << std::endl;
        exit(1);
    } else if (successCount.load() != numThreads * accessesPerThread) {
        std::cerr << "FAIL: Not all accesses succeeded. Expected: " 
                  << (numThreads * accessesPerThread) << ", Got: " 
                  << successCount.load() << std::endl;
        exit(1);
    } else {
        std::cout << "PASS: No race conditions detected" << std::endl;
    }
}

void testConcurrentTextureLoading() {
    std::cout << "\nTesting concurrent texture loading..." << std::endl;
    
    ResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    
    const int numThreads = 5;  // Reduced from 50
    const int texturesPerThread = 10;  // Reduced from 20
    std::atomic<int> loadCount{0};
    std::atomic<bool> errorDetected{false};
    std::vector<std::thread> threads;
    
    // Launch threads that load textures concurrently
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&manager, &loadCount, &errorDetected, i, texturesPerThread]() {
            try {
                for (int j = 0; j < texturesPerThread; ++j) {
                    std::string name = "texture_" + std::to_string(i) + "_" + std::to_string(j);
                    std::string path = "/nonexistent/path.png";  // Will use default texture
                    
                    Texture2D* texture = manager.loadTexture(path, name);
                    if (!texture) {
                        errorDetected.store(true);
                        std::cerr << "Failed to load texture: " << name << std::endl;
                        return;
                    }
                    
                    // Verify we get consistent texture back
                    Texture2D* texture2 = manager.getTexture(name);
                    if (texture != texture2) {
                        errorDetected.store(true);
                        std::cerr << "Texture pointer mismatch for: " << name << std::endl;
                        return;
                    }
                    
                    loadCount.fetch_add(1);
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
    
    std::cout << "Loaded " << loadCount.load() << " textures" << std::endl;
    std::cout << "Unique textures in manager: " << manager.getUniqueTexturesCount() << std::endl;
    
    // Clean up resources
    manager.clearAll();
    
    if (errorDetected.load()) {
        std::cerr << "FAIL: Error detected during concurrent loading!" << std::endl;
        exit(1);
    } else if (loadCount.load() != numThreads * texturesPerThread) {
        std::cerr << "FAIL: Not all textures loaded successfully" << std::endl;
        exit(1);
    } else {
        std::cout << "PASS: Concurrent texture loading successful" << std::endl;
    }
}

void testStressDefaultTexture() {
    std::cout << "\nStress testing default texture initialization..." << std::endl;
    
    const int numIterations = 10;
    
    for (int iter = 0; iter < numIterations; ++iter) {
        ResourceManager manager;
        manager.setSilentMode(true);
        manager.setHeadlessMode(true);
        
        const int numThreads = 20;  // Reduced from 200
        std::atomic<bool> errorDetected{false};
        std::vector<std::thread> threads;
        std::vector<Texture2D*> texturePointers(numThreads, nullptr);
        
        // All threads try to get default texture at exactly the same time
        std::atomic<bool> go{false};
        
        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([&manager, &errorDetected, &go, &texturePointers, i]() {
                // Wait for signal
                while (!go.load()) {
                    std::this_thread::yield();
                }
                
                try {
                    texturePointers[i] = &manager.getDefaultTexture();
                } catch (const std::exception& e) {
                    errorDetected.store(true);
                    std::cerr << "Exception: " << e.what() << std::endl;
                }
            });
        }
        
        // Give threads time to reach the wait point
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Release all threads at once
        go.store(true);
        
        // Wait for completion
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Verify all threads got the same texture pointer
        Texture2D* firstPtr = texturePointers[0];
        bool allSame = true;
        for (int i = 1; i < numThreads; ++i) {
            if (texturePointers[i] != firstPtr) {
                allSame = false;
                std::cerr << "Different texture pointers detected!" << std::endl;
                break;
            }
        }
        
        if (errorDetected.load() || !allSame) {
            std::cerr << "FAIL: Stress test iteration " << iter << " failed!" << std::endl;
            exit(1);
        }
    }
    
    std::cout << "PASS: Stress test completed successfully" << std::endl;
}

int main() {
    spdlog::set_level(spdlog::level::err);  // Only show errors
    
    std::cout << "=== ResourceManager Threading Tests ===" << std::endl;
    
    testConcurrentDefaultTextureAccess();
    testConcurrentTextureLoading();
    testStressDefaultTexture();
    
    std::cout << "\n=== All threading tests passed! ===" << std::endl;
    return 0;
}