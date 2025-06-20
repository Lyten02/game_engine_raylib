#include "resources/resource_manager.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <memory>
#include <spdlog/spdlog.h>

// Test the thread-safe initialization behavior of defaultTexture
// This test verifies that the default texture is only created once,
// even with multiple threads trying to access it simultaneously

int main() {
    spdlog::info("Testing thread-safe default texture initialization...");

    // Test 1: Single-threaded initialization
    {
        spdlog::info("\nTest 1: Single-threaded initialization");
        ResourceManager rm;
        rm.setHeadlessMode(true);
        rm.setSilentMode(true);
        
        // First access should create the texture
        Texture2D& tex1 = rm.getDefaultTexture();
        spdlog::info("First access: texture id={}, size={}x{}", tex1.id, tex1.width, tex1.height);
        
        // Second access should return the same texture
        Texture2D& tex2 = rm.getDefaultTexture();
        if (&tex1 == &tex2) {
            spdlog::info("✅ Same texture returned on second access");
        } else {
            spdlog::error("❌ Different textures returned!");
            return 1;
        }
    }

    // Test 2: Multi-threaded concurrent access
    {
        spdlog::info("\nTest 2: Multi-threaded concurrent access");
        ResourceManager rm;
        rm.setHeadlessMode(true);
        rm.setSilentMode(true);
        
        std::atomic<int> successCount{0};
        std::atomic<bool> allPointersEqual{true};
        Texture2D* firstTexture = nullptr;
        std::mutex resultMutex;
        
        // Launch 100 threads that all try to get the default texture
        std::vector<std::thread> threads;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 100; ++i) {
            threads.emplace_back([&rm, &successCount, &allPointersEqual, &firstTexture, &resultMutex]() {
                try {
                    Texture2D& tex = rm.getDefaultTexture();
                    successCount.fetch_add(1);
                    
                    // Check if all threads get the same texture
                    std::lock_guard<std::mutex> lock(resultMutex);
                    if (firstTexture == nullptr) {
                        firstTexture = &tex;
                    } else if (firstTexture != &tex) {
                        allPointersEqual.store(false);
                    }
                } catch (const std::exception& e) {
                    spdlog::error("Thread exception: {}", e.what());
                }
            });
        }
        
        // Wait for all threads
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        spdlog::info("Concurrent access completed in {} ms", duration);
        spdlog::info("Success count: {}/100", successCount.load());
        
        if (successCount == 100 && allPointersEqual) {
            spdlog::info("✅ All threads got the same texture pointer");
        } else {
            spdlog::error("❌ Concurrent access failed - not all threads got same texture");
            return 1;
        }
    }

    // Test 3: Rapid sequential access from multiple threads
    {
        spdlog::info("\nTest 3: Rapid sequential access");
        ResourceManager rm;
        rm.setHeadlessMode(true);
        rm.setSilentMode(true);
        
        std::atomic<int> accessCount{0};
        std::atomic<bool> error{false};
        
        std::vector<std::thread> threads;
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&rm, &accessCount, &error]() {
                try {
                    for (int j = 0; j < 1000; ++j) {
                        Texture2D& tex = rm.getDefaultTexture();
                        if (tex.width != 64 || tex.height != 64) {
                            error.store(true);
                            break;
                        }
                        accessCount.fetch_add(1);
                    }
                } catch (...) {
                    error.store(true);
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        if (!error && accessCount == 10000) {
            spdlog::info("✅ 10,000 rapid accesses completed successfully");
        } else {
            spdlog::error("❌ Rapid access test failed");
            return 1;
        }
    }

    // Test 4: Multiple ResourceManager instances
    {
        spdlog::info("\nTest 4: Multiple ResourceManager instances");
        
        std::vector<std::unique_ptr<ResourceManager>> managers;
        std::vector<Texture2D*> textures;
        
        // Create 5 resource managers
        for (int i = 0; i < 5; ++i) {
            auto rm = std::make_unique<ResourceManager>();
            rm->setHeadlessMode(true);
            rm->setSilentMode(true);
            textures.push_back(&rm->getDefaultTexture());
            managers.push_back(std::move(rm));
        }
        
        // Each manager should have its own default texture
        bool allDifferent = true;
        for (size_t i = 0; i < textures.size(); ++i) {
            for (size_t j = i + 1; j < textures.size(); ++j) {
                if (textures[i] == textures[j]) {
                    allDifferent = false;
                    break;
                }
            }
        }
        
        if (allDifferent) {
            spdlog::info("✅ Each ResourceManager has its own default texture");
        } else {
            spdlog::error("❌ ResourceManagers are sharing textures!");
            return 1;
        }
    }

    spdlog::info("\n✅ All thread-safe initialization tests passed!");
    return 0;
}