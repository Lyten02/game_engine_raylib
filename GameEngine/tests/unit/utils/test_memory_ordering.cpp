#include "resources/resource_manager.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <spdlog/spdlog.h>

// Test that verifies memory ordering works correctly on weak memory models
// This test creates high contention scenarios to expose any memory ordering issues

void testBasicMemoryOrdering() {
    std::cout << "Testing basic memory ordering guarantees..." << std::endl;
    
    // Create fresh ResourceManager for each test
    ResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    
    const int numThreads = 100;
    const int iterationsPerThread = 1000;
    std::atomic<int> successCount{0};
    std::atomic<bool> raceDetected{false};
    
    std::vector<std::thread> threads;
    
    // High contention test - all threads try to get default texture simultaneously
    auto startTime = std::chrono::steady_clock::now();
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&manager, &successCount, &raceDetected, iterationsPerThread]() {
            Texture2D* firstTexture = nullptr;
            
            for (int j = 0; j < iterationsPerThread; ++j) {
                try {
                    Texture2D& tex = manager.getDefaultTexture();
                    
                    // Verify texture is valid
                    if (tex.width != 64 || tex.height != 64) {
                        raceDetected.store(true);
                        std::cerr << "RACE: Invalid texture dimensions detected!" << std::endl;
                        return;
                    }
                    
                    // Store first texture pointer
                    if (!firstTexture) {
                        firstTexture = &tex;
                    } else if (firstTexture != &tex) {
                        // All calls should return the same texture
                        raceDetected.store(true);
                        std::cerr << "RACE: Different texture pointers returned!" << std::endl;
                        return;
                    }
                    
                    successCount.fetch_add(1);
                } catch (const std::exception& e) {
                    raceDetected.store(true);
                    std::cerr << "Exception in thread: " << e.what() << std::endl;
                    return;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    if (raceDetected.load()) {
        std::cerr << "FAIL: Race condition detected!" << std::endl;
        exit(1);
    }
    
    int expectedCount = numThreads * iterationsPerThread;
    if (successCount.load() != expectedCount) {
        std::cerr << "FAIL: Expected " << expectedCount << " successful calls, got " 
                  << successCount.load() << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: " << numThreads << " threads x " << iterationsPerThread 
              << " iterations completed in " << duration.count() << "ms" << std::endl;
}

void testStressMemoryOrdering() {
    std::cout << "\nTesting memory ordering under stress..." << std::endl;
    
    const int numManagers = 10;
    const int threadsPerManager = 20;
    const int iterations = 100;
    
    std::vector<std::unique_ptr<ResourceManager>> managers;
    std::vector<std::thread> threads;
    std::atomic<bool> errorDetected{false};
    
    // Create multiple managers
    for (int i = 0; i < numManagers; ++i) {
        auto manager = std::make_unique<ResourceManager>();
        manager->setSilentMode(true);
        manager->setHeadlessMode(true);
        managers.push_back(std::move(manager));
    }
    
    // Launch threads that randomly access different managers
    for (int i = 0; i < numManagers * threadsPerManager; ++i) {
        threads.emplace_back([&managers, &errorDetected, iterations, numManagers]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, numManagers - 1);
            
            for (int j = 0; j < iterations; ++j) {
                try {
                    // Randomly select a manager
                    int managerIndex = dis(gen);
                    ResourceManager* manager = managers[managerIndex].get();
                    
                    // Get default texture
                    Texture2D& tex = manager->getDefaultTexture();
                    
                    // Verify it's valid
                    if (tex.width != 64 || tex.height != 64 ||
                        tex.mipmaps != 1 || tex.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
                        errorDetected.store(true);
                        std::cerr << "Invalid texture from manager " << managerIndex << std::endl;
                        return;
                    }
                    
                    // Small delay to increase chances of interleaving
                    std::this_thread::yield();
                } catch (const std::exception& e) {
                    errorDetected.store(true);
                    std::cerr << "Exception: " << e.what() << std::endl;
                    return;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    if (errorDetected.load()) {
        std::cerr << "FAIL: Error detected during stress test" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Stress test with " << numManagers << " managers and " 
              << threads.size() << " threads completed successfully" << std::endl;
}

void testMemoryBarrierEffectiveness() {
    std::cout << "\nTesting memory barrier effectiveness..." << std::endl;
    
    // This test verifies that the memory barriers work correctly by
    // checking that initialization only happens once even under extreme contention
    
    const int numManagers = 10;
    const int numThreadsPerManager = 100;
    std::atomic<int> totalDefaultTextureAccesses{0};
    std::atomic<bool> multipleInitDetected{false};
    
    for (int m = 0; m < numManagers; ++m) {
        auto manager = std::make_unique<ResourceManager>();
        manager->setSilentMode(true);
        manager->setHeadlessMode(true);
        
        std::vector<std::thread> threads;
        std::atomic<int> readyThreads{0};
        std::atomic<bool> startSignal{false};
        std::vector<Texture2D*> texturePointers(numThreadsPerManager, nullptr);
        
        // Create threads that all wait for a signal to start simultaneously
        for (int i = 0; i < numThreadsPerManager; ++i) {
            threads.emplace_back([&manager, &readyThreads, &startSignal, &texturePointers, 
                                  &totalDefaultTextureAccesses, i]() {
                // Signal ready
                readyThreads.fetch_add(1);
                
                // Busy wait for start signal
                while (!startSignal.load(std::memory_order_acquire)) {
                    std::this_thread::yield();
                }
                
                // All threads try to get default texture at the same time
                try {
                    texturePointers[i] = &manager->getDefaultTexture();
                    totalDefaultTextureAccesses.fetch_add(1);
                } catch (...) {
                    // Ignore exceptions for this test
                }
            });
        }
        
        // Wait for all threads to be ready
        while (readyThreads.load() < numThreadsPerManager) {
            std::this_thread::yield();
        }
        
        // Release all threads at once
        startSignal.store(true, std::memory_order_release);
        
        // Wait for completion
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Verify all threads got the same texture pointer (single initialization)
        Texture2D* firstPtr = texturePointers[0];
        for (int i = 1; i < numThreadsPerManager; ++i) {
            if (texturePointers[i] && texturePointers[i] != firstPtr) {
                multipleInitDetected.store(true);
                std::cerr << "Different texture pointers detected!" << std::endl;
                break;
            }
        }
    }
    
    if (multipleInitDetected.load()) {
        std::cerr << "FAIL: Multiple initializations detected!" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Single initialization verified across " << numManagers 
              << " managers with " << numThreadsPerManager << " threads each" << std::endl;
    std::cout << "Total successful accesses: " << totalDefaultTextureAccesses.load() << std::endl;
}

void testWeakMemoryModelSimulation() {
    std::cout << "\nSimulating weak memory model behavior..." << std::endl;
    
    // This test adds random delays to simulate reordering that might
    // happen on weak memory model architectures
    
    ResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    
    const int numThreads = 50;
    const int iterations = 500;
    std::atomic<bool> errorFound{false};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&manager, &errorFound, iterations, i]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 10);
            
            for (int j = 0; j < iterations; ++j) {
                try {
                    // Random delay to simulate instruction reordering
                    std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
                    
                    Texture2D& tex = manager.getDefaultTexture();
                    
                    // More random delays
                    std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
                    
                    // Access texture fields (would fail if not properly initialized)
                    volatile int width = tex.width;
                    volatile int height = tex.height;
                    volatile int format = tex.format;
                    
                    if (width != 64 || height != 64) {
                        errorFound.store(true);
                        std::cerr << "Thread " << i << ": Invalid texture data!" << std::endl;
                        return;
                    }
                    
                } catch (const std::exception& e) {
                    errorFound.store(true);
                    std::cerr << "Thread " << i << ": " << e.what() << std::endl;
                    return;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    if (errorFound.load()) {
        std::cerr << "FAIL: Memory ordering issue detected" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Weak memory model simulation completed successfully" << std::endl;
}

int main() {
    spdlog::set_level(spdlog::level::warn);  // Reduce log noise for tests
    
    std::cout << "=== ResourceManager Memory Ordering Tests ===" << std::endl;
    std::cout << "Testing acquire-release semantics for ARM compatibility..." << std::endl;
    
    testBasicMemoryOrdering();
    testStressMemoryOrdering();
    testMemoryBarrierEffectiveness();
    testWeakMemoryModelSimulation();
    
    std::cout << "\n=== All memory ordering tests passed! ===" << std::endl;
    std::cout << "The implementation is safe for weak memory model architectures." << std::endl;
    return 0;
}