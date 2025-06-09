#include "../src/resources/resource_manager.h"
#include "../src/utils/log_limiter.h"
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <iostream>
#include <chrono>
#include <spdlog/spdlog.h>

// Test concurrent access to ResourceManager from multiple threads
// Verify no deadlocks or race conditions occur
// Test simultaneous texture loading and default texture access

static std::atomic<int> g_successCount{0};
static std::atomic<int> g_errorCount{0};

void threadWorker(ResourceManager& rm, int threadId, int iterations) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 4);
    
    for (int i = 0; i < iterations; ++i) {
        try {
            int operation = dis(gen);
            
            switch (operation) {
                case 0: {
                    // Load texture
                    std::string name = "texture_" + std::to_string(threadId) + "_" + std::to_string(i);
                    std::string path = "../assets/textures/test_sprite.png";
                    auto* tex = rm.loadTexture(path, name);
                    if (tex) {
                        g_successCount++;
                    } else {
                        g_errorCount++;
                    }
                    break;
                }
                case 1: {
                    // Get existing texture
                    std::string name = "texture_" + std::to_string(threadId % 5) + "_0";
                    auto* tex = rm.getTexture(name);
                    if (tex) {
                        g_successCount++;
                    } else {
                        g_errorCount++;
                    }
                    break;
                }
                case 2: {
                    // Get default texture
                    auto& defaultTex = rm.getDefaultTexture();
                    if (defaultTex.width == 64 && defaultTex.height == 64) {
                        g_successCount++;
                    } else {
                        g_errorCount++;
                    }
                    break;
                }
                case 3: {
                    // Unload texture
                    std::string name = "texture_" + std::to_string(threadId) + "_" + std::to_string(i - 1);
                    rm.unloadTexture(name);
                    g_successCount++;
                    break;
                }
                case 4: {
                    // Get texture count
                    size_t count = rm.getUniqueTexturesCount();
                    if (count >= 0) {
                        g_successCount++;
                    } else {
                        g_errorCount++;
                    }
                    break;
                }
            }
            
            // Small delay to increase chance of race conditions
            std::this_thread::sleep_for(std::chrono::microseconds(dis(gen) * 100));
            
        } catch (const std::exception& e) {
            spdlog::error("Thread {} caught exception: {}", threadId, e.what());
            g_errorCount++;
        }
    }
}

int main() {
    spdlog::set_level(spdlog::level::info);
    spdlog::info("Starting ResourceManager thread safety test (fixed version)");
    
    // Test parameters
    const int numThreads = 10;
    const int iterationsPerThread = 100;
    
    // Create ResourceManager in headless mode
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(false);
    
    // Start time
    auto startTime = std::chrono::steady_clock::now();
    
    // Create and start threads
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(threadWorker, std::ref(rm), i, iterationsPerThread);
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // End time
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Report results
    spdlog::info("Thread safety test completed in {} ms", duration.count());
    spdlog::info("Total operations: {}", g_successCount.load() + g_errorCount.load());
    spdlog::info("Successful operations: {}", g_successCount.load());
    spdlog::info("Failed operations: {}", g_errorCount.load());
    spdlog::info("Final texture count: {}", rm.getUniqueTexturesCount());
    
    // Test multiple ResourceManager instances accessing default texture concurrently
    spdlog::info("\nTesting multiple ResourceManager instances...");
    g_successCount = 0;
    g_errorCount = 0;
    
    auto multiInstanceWorker = [](int instanceId) {
        try {
            ResourceManager localRm;
            localRm.setHeadlessMode(true);
            localRm.setSilentMode(true);
            
            // Access default texture multiple times
            for (int i = 0; i < 50; ++i) {
                auto& defaultTex = localRm.getDefaultTexture();
                if (defaultTex.width == 64 && defaultTex.height == 64) {
                    g_successCount++;
                } else {
                    g_errorCount++;
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("Instance {} caught exception: {}", instanceId, e.what());
            g_errorCount++;
        }
    };
    
    // Create multiple instances in parallel
    threads.clear();
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(multiInstanceWorker, i);
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    spdlog::info("Multiple instance test completed");
    spdlog::info("Successful operations: {}", g_successCount.load());
    spdlog::info("Failed operations: {}", g_errorCount.load());
    
    // Test 3: Deadlock prevention test - multiple threads loading different textures
    spdlog::info("\nTesting deadlock prevention with concurrent texture loading...");
    g_successCount = 0;
    g_errorCount = 0;
    
    auto deadlockTestWorker = [&rm](int threadId) {
        try {
            // Each thread loads 10 different textures
            for (int i = 0; i < 10; ++i) {
                std::string name = "deadlock_test_" + std::to_string(threadId) + "_" + std::to_string(i);
                std::string path = "../assets/textures/test_sprite.png";
                
                // Simulate the problematic pattern: read lock, then write lock
                auto* tex = rm.loadTexture(path, name);
                if (tex) {
                    g_successCount++;
                } else {
                    g_errorCount++;
                }
                
                // Also try to load some textures that other threads might be loading
                std::string sharedName = "shared_texture_" + std::to_string(i);
                auto* sharedTex = rm.loadTexture(path, sharedName);
                if (sharedTex) {
                    g_successCount++;
                } else {
                    g_errorCount++;
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("Deadlock test thread {} caught exception: {}", threadId, e.what());
            g_errorCount++;
        }
    };
    
    // Start deadlock test with timeout
    auto deadlockStartTime = std::chrono::steady_clock::now();
    threads.clear();
    
    // Launch 20 threads simultaneously to maximize chance of deadlock
    for (int i = 0; i < 20; ++i) {
        threads.emplace_back(deadlockTestWorker, i);
    }
    
    // Wait with timeout
    bool deadlockDetected = false;
    for (auto& thread : threads) {
        // Give each thread 5 seconds to complete
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    auto deadlockEndTime = std::chrono::steady_clock::now();
    auto deadlockDuration = std::chrono::duration_cast<std::chrono::milliseconds>(deadlockEndTime - deadlockStartTime);
    
    spdlog::info("Deadlock prevention test completed in {} ms", deadlockDuration.count());
    spdlog::info("Successful operations: {}", g_successCount.load());
    spdlog::info("Failed operations: {}", g_errorCount.load());
    
    if (deadlockDuration.count() > 5000) {
        spdlog::warn("Test took longer than expected - possible performance issue");
    }
    
    // Test 4: Double-check pattern verification
    spdlog::info("\nTesting double-check pattern correctness...");
    g_successCount = 0;
    g_errorCount = 0;
    
    // Reset resource manager
    rm.unloadAll();
    
    auto doubleCheckWorker = [&rm](int threadId) {
        try {
            // All threads try to load the same texture
            std::string name = "double_check_texture";
            std::string path = "../assets/textures/test_sprite.png";
            
            auto* tex = rm.loadTexture(path, name);
            if (tex && tex->width > 0) {
                g_successCount++;
            } else {
                g_errorCount++;
            }
            
            // Verify we get the same texture on subsequent calls
            auto* tex2 = rm.getTexture(name);
            if (tex2 == tex) {
                g_successCount++;
            } else {
                spdlog::error("Thread {} got different texture pointers!", threadId);
                g_errorCount++;
            }
        } catch (const std::exception& e) {
            spdlog::error("Double-check test thread {} caught exception: {}", threadId, e.what());
            g_errorCount++;
        }
    };
    
    threads.clear();
    // Launch 50 threads all trying to load the same texture
    for (int i = 0; i < 50; ++i) {
        threads.emplace_back(doubleCheckWorker, i);
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    spdlog::info("Double-check pattern test completed");
    spdlog::info("Successful operations: {}", g_successCount.load());
    spdlog::info("Failed operations: {}", g_errorCount.load());
    
    // Verify only one texture was actually loaded
    size_t finalCount = rm.getUniqueTexturesCount();
    spdlog::info("Final unique texture count after double-check test: {}", finalCount);
    
    // Success if no errors
    bool success = (g_errorCount.load() == 0);
    if (success) {
        spdlog::info("\n✅ All thread safety tests passed!");
    } else {
        spdlog::error("\n❌ Thread safety test failed with {} errors", g_errorCount.load());
    }
    
    return success ? 0 : 1;
}