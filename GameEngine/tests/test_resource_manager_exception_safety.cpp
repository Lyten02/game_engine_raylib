#include <iostream>
#include <memory>
#include <exception>
#include <chrono>
#include <thread>
#include "../src/resources/resource_manager.h"

// Mock exception class for testing
class MockRayLibException : public std::runtime_error {
public:
    MockRayLibException() : std::runtime_error("Mock RayLib initialization failure") {}
};

// Test fixture to simulate RayLib failures
class ExceptionTestResourceManager : public ResourceManager {
public:
    bool shouldThrowException = false;
    
    void createDefaultTexture() override {
        if (shouldThrowException) {
            throw MockRayLibException();
        }
        // Call parent implementation
        ResourceManager::createDefaultTexture();
    }
};

bool testExceptionSafety() {
    std::cout << "Testing exception safety in getDefaultTexture()..." << std::endl;
    
    try {
        // Test 1: Normal operation
        {
            ExceptionTestResourceManager rm;
            rm.setHeadlessMode(true);
            rm.setSilentMode(true);
            
            Texture2D& tex = rm.getDefaultTexture();
            if (tex.width != 64 || tex.height != 64) {
                std::cerr << "✗ Normal operation failed: incorrect texture dimensions" << std::endl;
                return false;
            }
            std::cout << "✓ Normal operation successful" << std::endl;
        }
        
        // Test 2: Exception during creation with fallback
        {
            ExceptionTestResourceManager rm;
            rm.setHeadlessMode(false);
            rm.setSilentMode(true);
            rm.setRayLibInitialized(true);
            rm.shouldThrowException = true;
            
            // This should catch the exception and create fallback texture
            Texture2D& tex = rm.getDefaultTexture();
            if (tex.width != 64 || tex.height != 64) {
                std::cerr << "✗ Exception fallback failed: incorrect texture dimensions" << std::endl;
                return false;
            }
            if (tex.id != 0) {
                std::cerr << "✗ Exception fallback failed: texture ID should be 0 for dummy texture" << std::endl;
                return false;
            }
            std::cout << "✓ Exception caught and fallback texture created" << std::endl;
        }
        
        // Test 3: Multiple calls after exception
        {
            ExceptionTestResourceManager rm;
            rm.setHeadlessMode(false);
            rm.setSilentMode(true);
            rm.setRayLibInitialized(true);
            rm.shouldThrowException = true;
            
            // First call - triggers exception and fallback
            Texture2D& tex1 = rm.getDefaultTexture();
            
            // Second call - should return same texture without calling createDefaultTexture again
            rm.shouldThrowException = false; // Would throw if called again
            Texture2D& tex2 = rm.getDefaultTexture();
            
            if (&tex1 != &tex2) {
                std::cerr << "✗ std::call_once failed: different textures returned" << std::endl;
                return false;
            }
            std::cout << "✓ std::call_once prevents multiple initialization attempts" << std::endl;
        }
        
        // Test 4: Thread safety with exceptions
        {
            ExceptionTestResourceManager rm;
            rm.setHeadlessMode(false);
            rm.setSilentMode(true);
            rm.setRayLibInitialized(true);
            rm.shouldThrowException = true;
            
            std::vector<std::thread> threads;
            std::vector<Texture2D*> results(10, nullptr);
            std::atomic<int> exceptionCount{0};
            
            // Launch multiple threads trying to get default texture
            for (int i = 0; i < 10; ++i) {
                threads.emplace_back([&rm, &results, &exceptionCount, i]() {
                    try {
                        results[i] = &rm.getDefaultTexture();
                    } catch (const std::exception& e) {
                        exceptionCount++;
                    }
                });
            }
            
            // Wait for all threads
            for (auto& t : threads) {
                t.join();
            }
            
            // All threads should get the same texture
            Texture2D* firstTexture = nullptr;
            for (auto* tex : results) {
                if (tex != nullptr) {
                    if (firstTexture == nullptr) {
                        firstTexture = tex;
                    } else if (tex != firstTexture) {
                        std::cerr << "✗ Thread safety failed: different textures returned" << std::endl;
                        return false;
                    }
                }
            }
            
            if (firstTexture == nullptr) {
                std::cerr << "✗ All threads failed to get texture" << std::endl;
                return false;
            }
            
            if (exceptionCount > 0) {
                std::cerr << "✗ Some threads received exceptions instead of fallback" << std::endl;
                return false;
            }
            
            std::cout << "✓ Thread-safe exception handling successful" << std::endl;
        }
        
        std::cout << "✅ All exception safety tests passed!" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "✗ Unexpected exception: " << e.what() << std::endl;
        return false;
    }
}

// Test class that fails a specific number of times before succeeding
class FailingResourceManager : public ResourceManager {
private:
    mutable std::atomic<int> attemptCount{0};
    int failuresBeforeSuccess;

public:
    FailingResourceManager(int failures) : failuresBeforeSuccess(failures) {
        setHeadlessMode(true);
        setSilentMode(true);
    }

    void createDefaultTexture() override {
        int attempt = attemptCount.fetch_add(1);
        
        if (attempt < failuresBeforeSuccess) {
            throw std::runtime_error("Simulated failure attempt " + std::to_string(attempt + 1));
        }
        
        // Success on final attempt
        ResourceManager::createDefaultTexture();
    }

    int getAttemptCount() const { return attemptCount.load(); }
};

bool testCallOnceRetryBehavior() {
    std::cout << "\nTesting call_once retry behavior..." << std::endl;
    
    try {
        // Test that call_once properly retries after exceptions
        FailingResourceManager rm(1);  // Fail once, then succeed
        
        auto& texture = rm.getDefaultTexture();
        
        // Verify it tried twice
        if (rm.getAttemptCount() == 2) {
            std::cout << "✓ call_once properly retried after exception" << std::endl;
        } else {
            std::cerr << "✗ call_once did not retry properly (attempts: " << rm.getAttemptCount() << ")" << std::endl;
            return false;
        }
        
        // Call again - should not retry
        auto& texture2 = rm.getDefaultTexture();
        if (rm.getAttemptCount() == 2) {
            std::cout << "✓ call_once did not retry on subsequent calls" << std::endl;
        } else {
            std::cerr << "✗ call_once incorrectly retried (attempts: " << rm.getAttemptCount() << ")" << std::endl;
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "✗ Unexpected exception in retry test: " << e.what() << std::endl;
        return false;
    }
}

int main() {
    std::cout << "=== ResourceManager Exception Safety Test ===" << std::endl;
    
    bool success = testExceptionSafety();
    
    if (success) {
        success = testCallOnceRetryBehavior();
    }
    
    if (success) {
        std::cout << "\n✅ All exception safety and retry tests passed!" << std::endl;
    }
    
    return success ? 0 : 1;
}