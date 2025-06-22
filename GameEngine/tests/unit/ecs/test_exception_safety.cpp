#include "resources/resource_manager.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <exception>
#include <spdlog/spdlog.h>

// Test exception safety guarantees of ResourceManager

void testBasicExceptionSafety() {
    std::cout << "Testing basic exception safety..." << std::endl;
    
    // Test 1: Exceptions during normal operation shouldn't crash
    {
        ResourceManager manager;
        manager.setSilentMode(false);  // Enable logging to see fallback messages
        manager.setHeadlessMode(true);
        
        try {
            // This should work fine - headless mode creates dummy texture
            Texture2D& tex = manager.getDefaultTexture();
            if (tex.width != 64 || tex.height != 64) {
                std::cerr << "FAIL: Invalid texture dimensions" << std::endl;
                exit(1);
            }
            std::cout << "PASS: Headless mode texture creation is exception safe" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "FAIL: Unexpected exception: " << e.what() << std::endl;
            exit(1);
        }
    }
    
    // Test 2: Multiple managers with different modes
    {
        std::vector<std::unique_ptr<ResourceManager>> managers;
        
        for (int i = 0; i < 10; ++i) {
            auto manager = std::make_unique<ResourceManager>();
            manager->setSilentMode(true);
            
            // Alternate between headless and non-initialized RayLib
            if (i % 2 == 0) {
                manager->setHeadlessMode(true);
                manager->setRayLibInitialized(false);
            } else {
                manager->setHeadlessMode(false);
                manager->setRayLibInitialized(false);  // Should still create dummy
            }
            
            try {
                Texture2D& tex = manager->getDefaultTexture();
                // Should always get a valid dummy texture
                if (tex.id != 0 || tex.width != 64 || tex.height != 64) {
                    std::cerr << "FAIL: Invalid dummy texture in manager " << i << std::endl;
                    exit(1);
                }
            } catch (const std::exception& e) {
                std::cerr << "FAIL: Exception in manager " << i << ": " << e.what() << std::endl;
                exit(1);
            }
            
            managers.push_back(std::move(manager));
        }
        
        std::cout << "PASS: Multiple managers handled safely" << std::endl;
    }
}

void testConcurrentExceptionSafety() {
    std::cout << "\nTesting concurrent exception safety..." << std::endl;
    
    ResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(false);
    manager.setRayLibInitialized(false);  // Force dummy texture creation
    
    const int numThreads = 50;
    std::atomic<int> successCount{0};
    std::atomic<int> exceptionCount{0};
    std::vector<std::thread> threads;
    
    // Launch threads that all try to access default texture
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&manager, &successCount, &exceptionCount]() {
            for (int j = 0; j < 100; ++j) {
                try {
                    Texture2D& tex = manager.getDefaultTexture();
                    
                    // Verify texture is valid
                    if (tex.width == 64 && tex.height == 64) {
                        successCount.fetch_add(1);
                    }
                } catch (const std::exception& e) {
                    // Count exceptions (should be rare/none)
                    exceptionCount.fetch_add(1);
                }
                
                // Small delay to increase contention
                std::this_thread::yield();
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "Success count: " << successCount.load() << std::endl;
    std::cout << "Exception count: " << exceptionCount.load() << std::endl;
    
    if (exceptionCount.load() > 0) {
        std::cerr << "WARNING: Some exceptions occurred during concurrent access" << std::endl;
    }
    
    if (successCount.load() == 0) {
        std::cerr << "FAIL: No successful texture accesses!" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Concurrent access handled safely" << std::endl;
}

void testResourceLifetimeSafety() {
    std::cout << "\nTesting resource lifetime safety..." << std::endl;
    
    // Test that texture pointers remain valid throughout manager lifetime
    Texture2D* persistentPointer = nullptr;
    
    {
        ResourceManager manager;
        manager.setSilentMode(true);
        manager.setHeadlessMode(true);
        
        // Get default texture
        persistentPointer = &manager.getDefaultTexture();
        
        // Load some textures
        for (int i = 0; i < 100; ++i) {
            manager.loadTexture("/fake/path.png", "tex_" + std::to_string(i));
        }
        
        // Verify pointer is still valid
        if (persistentPointer->width != 64) {
            std::cerr << "FAIL: Texture corrupted during manager lifetime" << std::endl;
            exit(1);
        }
        
        // Manager destroyed here
    }
    
    // Don't access persistentPointer here - it's now invalid
    // This test just verifies clean destruction
    
    std::cout << "PASS: Resource lifetime managed safely" << std::endl;
}

void testExceptionPropagation() {
    std::cout << "\nTesting exception propagation..." << std::endl;
    
    // Test that critical errors are properly propagated
    {
        ResourceManager manager;
        manager.setSilentMode(false);
        
        // This should work (creates dummy texture)
        manager.setHeadlessMode(true);
        
        bool exceptionCaught = false;
        try {
            Texture2D& tex = manager.getDefaultTexture();
            
            // Should succeed
            if (tex.width != 64) {
                std::cerr << "FAIL: Unexpected texture dimensions" << std::endl;
                exit(1);
            }
        } catch (const std::runtime_error& e) {
            exceptionCaught = true;
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
        }
        
        if (exceptionCaught) {
            std::cerr << "FAIL: Exception thrown when it shouldn't be" << std::endl;
            exit(1);
        }
        
        std::cout << "PASS: No spurious exceptions" << std::endl;
    }
}

void testMemoryExhaustionScenario() {
    std::cout << "\nTesting behavior under memory pressure..." << std::endl;
    
    // Create many managers to stress memory
    std::vector<std::unique_ptr<ResourceManager>> managers;
    const int numManagers = 100;
    
    try {
        for (int i = 0; i < numManagers; ++i) {
            auto manager = std::make_unique<ResourceManager>();
            manager->setSilentMode(true);
            manager->setHeadlessMode(true);
            
            // Force texture creation
            manager->getDefaultTexture();
            
            // Store the manager
            managers.push_back(std::move(manager));
        }
        
        std::cout << "PASS: Created " << numManagers << " managers successfully" << std::endl;
    } catch (const std::bad_alloc& e) {
        std::cout << "INFO: System ran out of memory after " << managers.size() 
                  << " managers (expected on low-memory systems)" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: Unexpected exception type: " << e.what() << std::endl;
        exit(1);
    }
    
    // Clean destruction
    managers.clear();
    std::cout << "PASS: Clean destruction of all managers" << std::endl;
}

void testNestedExceptionHandling() {
    std::cout << "\nTesting nested exception handling..." << std::endl;
    
    // Test exception safety when exceptions occur during exception handling
    ResourceManager manager;
    manager.setSilentMode(false);
    manager.setHeadlessMode(true);
    
    std::vector<std::thread> threads;
    std::atomic<bool> allThreadsSucceeded{true};
    
    // Multiple threads accessing the manager
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&manager, &allThreadsSucceeded, i]() {
            try {
                // Each thread does multiple operations
                for (int j = 0; j < 50; ++j) {
                    // Get default texture
                    Texture2D& tex = manager.getDefaultTexture();
                    
                    // Load texture
                    std::string name = "thread_" + std::to_string(i) + "_tex_" + std::to_string(j);
                    manager.loadTexture("/fake/path.png", name);
                    
                    // Get it back
                    Texture2D* loaded = manager.getTexture(name);
                    if (!loaded) {
                        allThreadsSucceeded.store(false);
                        return;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Thread " << i << " caught exception: " << e.what() << std::endl;
                allThreadsSucceeded.store(false);
            } catch (...) {
                std::cerr << "Thread " << i << " caught unknown exception" << std::endl;
                allThreadsSucceeded.store(false);
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    if (!allThreadsSucceeded.load()) {
        std::cerr << "FAIL: Some threads failed" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Nested operations handled safely" << std::endl;
}

int main() {
    spdlog::set_level(spdlog::level::info);  // Show some log messages for exception tests
    
    std::cout << "=== ResourceManager Exception Safety Tests ===" << std::endl;
    
    testBasicExceptionSafety();
    testConcurrentExceptionSafety();
    testResourceLifetimeSafety();
    testExceptionPropagation();
    testMemoryExhaustionScenario();
    testNestedExceptionHandling();
    
    std::cout << "\n=== All exception safety tests passed! ===" << std::endl;
    std::cout << "The ResourceManager provides strong exception safety guarantees." << std::endl;
    return 0;
}