#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include "resources/resource_manager.h"

// Test class that can inject exceptions
class TestResourceManager : public ResourceManager {
public:
    std::atomic<bool> shouldThrowInCreateDefault{false};
    std::atomic<int> createDefaultCallCount{0};
    std::atomic<bool> shouldThrowInMapInsert{false};
    std::atomic<bool> shouldThrowInUnloadTexture{false};
    std::atomic<bool> shouldFailToCreateTexture{false};
    
    // Override to inject exceptions
    void createDefaultTextureThreadSafe() override {
        createDefaultCallCount++;
        
        if (shouldThrowInCreateDefault.load()) {
            throw std::runtime_error("Injected exception in createDefaultTextureThreadSafe");
        }
        
        if (shouldFailToCreateTexture.load()) {
            // Simulate failure to create texture - defaultTexture remains null
            defaultTexture = nullptr;
            // This will cause the post-condition check to throw
            if (!defaultTexture) {
                throw std::runtime_error("Cannot create default texture - system failure");
            }
        }
        
        // Simple implementation for testing
        if (!defaultTexture) {
            defaultTexture = std::make_unique<Texture2D>();
            defaultTexture->id = 1;
            defaultTexture->width = 64;
            defaultTexture->height = 64;
            defaultTexture->mipmaps = 1;
            defaultTexture->format = 7; // PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
        }
    }
};

bool testExceptionSafetyInGetDefaultTexture() {
    std::cout << "Testing exception safety in getDefaultTexture..." << std::endl;
    
    TestResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    manager.setRayLibInitialized(false);
    
    // Enable exception injection
    manager.shouldThrowInCreateDefault = true;
    
    // First thread will throw exception
    std::atomic<bool> firstThreadDone{false};
    std::atomic<bool> secondThreadStarted{false};
    std::atomic<bool> deadlockDetected{false};
    
    std::thread firstThread([&]() {
        try {
            manager.getDefaultTexture();
        } catch (const std::exception& e) {
            std::cout << "First thread caught expected exception: " << e.what() << std::endl;
        }
        firstThreadDone = true;
    });
    
    // Give first thread time to acquire lock and throw
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Second thread should not deadlock
    std::thread secondThread([&]() {
        secondThreadStarted = true;
        
        // Try to get default texture with timeout
        auto start = std::chrono::steady_clock::now();
        
        try {
            // Disable exception for second attempt
            manager.shouldThrowInCreateDefault = false;
            manager.getDefaultTexture();
        } catch (...) {
            // Ignore any exceptions
        }
        
        auto duration = std::chrono::steady_clock::now() - start;
        if (duration > std::chrono::seconds(2)) {
            deadlockDetected = true;
        }
    });
    
    // Monitor threads with timeout
    auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!firstThreadDone || !secondThreadStarted) {
        if (std::chrono::steady_clock::now() > timeout) {
            std::cerr << "FAIL: Timeout waiting for threads" << std::endl;
            deadlockDetected = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    firstThread.join();
    secondThread.join();
    
    if (deadlockDetected) {
        std::cerr << "FAIL: Deadlock detected!" << std::endl;
        return false;
    }
    
    std::cout << "PASS: No deadlock after exception" << std::endl;
    return true;
}

bool testConcurrentExceptions() {
    std::cout << "\nTesting concurrent exceptions..." << std::endl;
    
    TestResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    manager.setRayLibInitialized(false);
    
    const int numThreads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    std::atomic<int> exceptionCount{0};
    
    // Half threads will throw, half won't
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([&, i]() {
            manager.shouldThrowInCreateDefault = (i % 2 == 0);
            
            try {
                manager.getDefaultTexture();
                successCount++;
            } catch (const std::exception& e) {
                exceptionCount++;
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "Success count: " << successCount << ", Exception count: " << exceptionCount << std::endl;
    
    if (successCount + exceptionCount != numThreads) {
        std::cerr << "FAIL: Some threads didn't complete" << std::endl;
        return false;
    }
    
    std::cout << "PASS: All threads completed without deadlock" << std::endl;
    return true;
}

bool testSharedMutexExceptionSafety() {
    std::cout << "\nTesting shared_mutex exception safety..." << std::endl;
    
    TestResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    manager.setRayLibInitialized(false);
    
    const int numReaders = 5;
    const int numWriters = 2;
    std::vector<std::thread> threads;
    std::atomic<bool> deadlockDetected{false};
    std::atomic<int> completedThreads{0};
    
    // Start reader threads
    for (int i = 0; i < numReaders; i++) {
        threads.emplace_back([&, i]() {
            try {
                // Repeatedly try to get textures
                for (int j = 0; j < 10; j++) {
                    auto* tex = manager.getTexture("test" + std::to_string(i));
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            } catch (...) {
                // Ignore exceptions
            }
            completedThreads++;
        });
    }
    
    // Start writer threads
    for (int i = 0; i < numWriters; i++) {
        threads.emplace_back([&, i]() {
            try {
                // Try to load textures
                for (int j = 0; j < 5; j++) {
                    manager.loadTexture("path" + std::to_string(j), "test" + std::to_string(i));
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            } catch (...) {
                // Ignore exceptions
            }
            completedThreads++;
        });
    }
    
    // Monitor with timeout
    auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (completedThreads < numReaders + numWriters) {
        if (std::chrono::steady_clock::now() > timeout) {
            deadlockDetected = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    if (deadlockDetected) {
        std::cerr << "FAIL: Deadlock detected in shared_mutex!" << std::endl;
        return false;
    }
    
    std::cout << "PASS: No deadlock with shared_mutex" << std::endl;
    return true;
}

bool testNestedLockException() {
    std::cout << "\nTesting nested lock exception safety..." << std::endl;
    
    // Create a scenario where we might have nested locks
    TestResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    manager.setRayLibInitialized(false);
    
    std::atomic<bool> testPassed{true};
    std::vector<std::thread> threads;
    
    // Thread 1: Continuously unload all
    threads.emplace_back([&]() {
        for (int i = 0; i < 10; i++) {
            try {
                manager.unloadAll();
            } catch (...) {
                testPassed = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    
    // Thread 2: Load textures
    threads.emplace_back([&]() {
        for (int i = 0; i < 20; i++) {
            try {
                manager.loadTexture("test.png", "texture" + std::to_string(i));
            } catch (...) {
                // Expected - might fail due to concurrent unload
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });
    
    // Thread 3: Get textures
    threads.emplace_back([&]() {
        for (int i = 0; i < 30; i++) {
            try {
                manager.getTexture("texture" + std::to_string(i % 20));
            } catch (...) {
                // Expected
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    if (!testPassed) {
        std::cerr << "FAIL: Exception in unloadAll" << std::endl;
        return false;
    }
    
    std::cout << "PASS: Nested operations completed safely" << std::endl;
    return true;
}

bool testRepeatedExceptionProblem() {
    std::cout << "\nTesting repeated exception problem..." << std::endl;
    
    TestResourceManager manager;
    manager.setSilentMode(true);
    manager.setHeadlessMode(true);
    manager.setRayLibInitialized(false);
    
    // Enable failure mode
    manager.shouldFailToCreateTexture = true;
    
    const int numThreads = 5;
    std::vector<std::thread> threads;
    std::atomic<int> exceptionCount{0};
    std::atomic<int> callCount{0};
    
    // Launch multiple threads that will all try to initialize
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 3; j++) {
                try {
                    manager.getDefaultTexture();
                } catch (const std::exception& e) {
                    exceptionCount++;
                }
                callCount++;
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "Create default called: " << manager.createDefaultCallCount << " times" << std::endl;
    std::cout << "Exceptions thrown: " << exceptionCount << std::endl;
    std::cout << "Total calls: " << callCount << std::endl;
    
    // The problem: createDefaultTextureThreadSafe is called multiple times
    // because defaultTextureInitialized remains false after exception
    if (manager.createDefaultCallCount > 1) {
        std::cerr << "FAIL: createDefaultTextureThreadSafe called " 
                  << manager.createDefaultCallCount << " times (expected 1)" << std::endl;
        std::cerr << "This demonstrates the exception safety problem!" << std::endl;
        return false;
    }
    
    std::cout << "PASS: Exception handled correctly" << std::endl;
    return true;
}

int main() {
    std::cout << "Running ResourceManager exception safety deadlock test..." << std::endl;
    
    bool allTestsPassed = true;
    
    // Test 1: Exception in getDefaultTexture
    if (!testExceptionSafetyInGetDefaultTexture()) {
        allTestsPassed = false;
    }
    
    // Test 2: Concurrent exceptions
    if (!testConcurrentExceptions()) {
        allTestsPassed = false;
    }
    
    // Test 3: Shared mutex exception safety
    if (!testSharedMutexExceptionSafety()) {
        allTestsPassed = false;
    }
    
    // Test 4: Nested lock exception
    if (!testNestedLockException()) {
        allTestsPassed = false;
    }
    
    // Test 5: Repeated exception problem
    if (!testRepeatedExceptionProblem()) {
        allTestsPassed = false;
    }
    
    if (allTestsPassed) {
        std::cout << "\nAll tests passed!" << std::endl;
        return 0;
    } else {
        std::cerr << "\nSome tests failed!" << std::endl;
        return 1;
    }
}