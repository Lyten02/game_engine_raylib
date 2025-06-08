#include "../src/resources/resource_manager.h"
#include <iostream>
#include <atomic>
#include <spdlog/spdlog.h>

// Test that std::call_once properly retries after exceptions
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
        spdlog::info("createDefaultTexture attempt #{}", attempt + 1);
        
        if (attempt < failuresBeforeSuccess) {
            throw std::runtime_error("Simulated failure attempt " + std::to_string(attempt + 1));
        }
        
        // Success on final attempt
        ResourceManager::createDefaultTexture();
    }

    int getAttemptCount() const { return attemptCount.load(); }
};

int main() {
    spdlog::info("Testing std::call_once retry behavior...");

    // Test 1: Should retry 3 times before success
    {
        spdlog::info("\nTest 1: 2 failures, then success");
        FailingResourceManager rm(2);  // Fail 2 times, succeed on 3rd
        
        try {
            auto& texture = rm.getDefaultTexture();
            spdlog::info("✅ Got default texture after {} attempts", rm.getAttemptCount());
            
            // Call again - should NOT retry (call_once already succeeded)
            auto& texture2 = rm.getDefaultTexture();
            if (rm.getAttemptCount() == 3) {
                spdlog::info("✅ call_once didn't retry - correct behavior");
            } else {
                spdlog::error("❌ call_once retried when it shouldn't");
                return 1;
            }
        } catch (const std::exception& e) {
            spdlog::error("❌ Test 1 failed: {}", e.what());
            return 1;
        }
    }

    // Test 2: Multiple instances should each try independently
    {
        spdlog::info("\nTest 2: Multiple ResourceManager instances");
        FailingResourceManager rm1(1);  // Fail once, succeed on 2nd
        FailingResourceManager rm2(0);  // Succeed immediately
        
        try {
            auto& tex1 = rm1.getDefaultTexture();
            auto& tex2 = rm2.getDefaultTexture();
            
            if (rm1.getAttemptCount() == 2 && rm2.getAttemptCount() == 1) {
                spdlog::info("✅ Multiple instances work independently");
            } else {
                spdlog::error("❌ Instance counts: rm1={}, rm2={}", rm1.getAttemptCount(), rm2.getAttemptCount());
                return 1;
            }
        } catch (const std::exception& e) {
            spdlog::error("❌ Test 2 failed: {}", e.what());
            return 1;
        }
    }

    spdlog::info("\n✅ All call_once retry tests passed!");
    return 0;
}