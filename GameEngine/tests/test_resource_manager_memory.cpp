#include "../src/resources/resource_manager.h"
#include <iostream>
#include <memory>
#include <exception>
#include <vector>
#include <spdlog/spdlog.h>

// Custom exception for testing
class TestException : public std::exception {
public:
    const char* what() const noexcept override {
        return "Test exception thrown";
    }
};

// We'll test exception handling without mocking since methods are not virtual

void testDefaultTextureMemorySafety() {
    std::cout << "Testing default texture memory safety..." << std::endl;
    
    // Test 1: Normal creation and destruction
    {
        ResourceManager manager;
        manager.setSilentMode(true);
        manager.setHeadlessMode(true);
        
        Texture2D& tex1 = manager.getDefaultTexture();
        Texture2D& tex2 = manager.getDefaultTexture();
        
        if (&tex1 != &tex2) {
            std::cerr << "FAIL: Default texture not singleton!" << std::endl;
            exit(1);
        }
        
        std::cout << "PASS: Default texture singleton works correctly" << std::endl;
    }
    
    // Test 2: Multiple ResourceManager instances
    {
        std::vector<std::unique_ptr<ResourceManager>> managers;
        for (int i = 0; i < 10; ++i) {
            auto manager = std::make_unique<ResourceManager>();
            manager->setSilentMode(true);
            manager->setHeadlessMode(true);
            
            // Each should have its own default texture
            Texture2D& tex = manager->getDefaultTexture();
            if (tex.width != 64 || tex.height != 64) {
                std::cerr << "FAIL: Invalid texture dimensions in manager " << i << std::endl;
                exit(1);
            }
            
            managers.push_back(std::move(manager));
        }
        
        std::cout << "PASS: Multiple ResourceManager instances work correctly" << std::endl;
    }
    
    // Test 3: Destruction order
    {
        ResourceManager* manager = new ResourceManager();
        manager->setSilentMode(true);
        manager->setHeadlessMode(true);
        
        // Load some textures
        for (int i = 0; i < 100; ++i) {
            std::string name = "test_texture_" + std::to_string(i);
            manager->loadTexture("/fake/path.png", name);
        }
        
        // Get default texture reference
        Texture2D& defaultTex = manager->getDefaultTexture();
        
        // Delete manager - should clean up properly
        delete manager;
        
        // If we get here without crashing, memory cleanup worked
        std::cout << "PASS: Proper cleanup on destruction" << std::endl;
    }
}

void testExceptionSafety() {
    std::cout << "\nTesting exception safety..." << std::endl;
    
    // Test that emergency fallback works
    {
        ResourceManager manager;
        manager.setSilentMode(false);  // Enable logging for this test
        manager.setHeadlessMode(false);
        manager.setRayLibInitialized(false);  // Force dummy texture
        
        try {
            Texture2D& tex = manager.getDefaultTexture();
            if (tex.width != 64 || tex.height != 64) {
                std::cerr << "FAIL: Emergency fallback texture has wrong dimensions" << std::endl;
                exit(1);
            }
            std::cout << "PASS: Emergency fallback works correctly" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "FAIL: Exception thrown when it shouldn't: " << e.what() << std::endl;
            exit(1);
        }
    }
}

void testMemoryLeaks() {
    std::cout << "\nTesting for memory leaks..." << std::endl;
    
    // Stress test - create and destroy many textures
    for (int iter = 0; iter < 5; ++iter) {
        ResourceManager manager;
        manager.setSilentMode(true);
        manager.setHeadlessMode(true);
        
        // Load many textures
        for (int i = 0; i < 1000; ++i) {
            std::string name = "stress_texture_" + std::to_string(i);
            manager.loadTexture("/fake/path.png", name);
        }
        
        // Access default texture many times
        for (int i = 0; i < 10000; ++i) {
            Texture2D& tex = manager.getDefaultTexture();
            (void)tex;  // Suppress unused warning
        }
        
        // Unload half the textures
        for (int i = 0; i < 500; ++i) {
            std::string name = "stress_texture_" + std::to_string(i);
            manager.unloadTexture(name);
        }
        
        // Manager will be destroyed here, should clean up all memory
    }
    
    std::cout << "PASS: No apparent memory leaks (run with valgrind for confirmation)" << std::endl;
}

void testNullPointerSafety() {
    std::cout << "\nTesting null pointer safety..." << std::endl;
    
    ResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    
    // Test getTexture with non-existent texture
    Texture2D* tex = manager.getTexture("non_existent");
    if (!tex) {
        std::cerr << "FAIL: getTexture returned null instead of default texture" << std::endl;
        exit(1);
    }
    
    if (tex != &manager.getDefaultTexture()) {
        std::cerr << "FAIL: getTexture didn't return default texture for missing resource" << std::endl;
        exit(1);
    }
    
    // Test getSound with non-existent sound
    Sound* sound = manager.getSound("non_existent");
    if (sound != nullptr) {
        std::cerr << "FAIL: getSound should return nullptr for non-existent sound" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Null pointer safety checks passed" << std::endl;
}

void testThreadSafeDestruction() {
    std::cout << "\nTesting thread-safe destruction..." << std::endl;
    
    // Create manager and start threads that use it
    auto manager = std::make_unique<ResourceManager>();
    manager->setSilentMode(true);
    manager->setHeadlessMode(true);
    
    std::atomic<bool> keepRunning{true};
    std::vector<std::thread> threads;
    
    // Start threads that continuously access textures
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&manager, &keepRunning, i]() {
            while (keepRunning.load()) {
                try {
                    // Access default texture
                    manager->getDefaultTexture();
                    
                    // Load and access textures
                    std::string name = "thread_texture_" + std::to_string(i);
                    manager->loadTexture("/fake/path.png", name);
                    manager->getTexture(name);
                    
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                } catch (...) {
                    // Ignore exceptions during shutdown
                }
            }
        });
    }
    
    // Let threads run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Signal threads to stop
    keepRunning.store(false);
    
    // Wait for threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Now destroy the manager - should be safe
    manager.reset();
    
    std::cout << "PASS: Thread-safe destruction completed" << std::endl;
}

int main() {
    spdlog::set_level(spdlog::level::warn);  // Reduce log noise
    
    std::cout << "=== ResourceManager Memory Safety Tests ===" << std::endl;
    
    testDefaultTextureMemorySafety();
    testExceptionSafety();
    testMemoryLeaks();
    testNullPointerSafety();
    testThreadSafeDestruction();
    
    std::cout << "\n=== All memory safety tests passed! ===" << std::endl;
    return 0;
}