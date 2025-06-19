#include "../src/utils/log_limiter.h"
#include <iostream>
#include <thread>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

int main() {
    // Setup logging
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::info);
    
    std::cout << "Testing LogLimiter with generic keys functionality\n" << std::endl;
    
    // Configure LogLimiter to limit messages to 3 occurrences
    GameEngine::LogLimiter::configure(3, 60, true);
    
    std::cout << "\n--- Testing rate limiting with generic keys ---" << std::endl;
    
    // Test 1: Basic rate limiting with same key
    std::cout << "\nTest 1: Same message repeated (should see 3 then suppression)" << std::endl;
    for (int i = 0; i < 10; ++i) {
        GameEngine::LogLimiter::info("test_key", "Test message {}", i);
    }
    
    // Test 2: Different keys should have separate limits
    std::cout << "\nTest 2: Different keys (each should get 3 messages)" << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::string key = "key_" + std::to_string(i % 2);
        GameEngine::LogLimiter::warn(key, "Message for {} - iteration {}", key, i);
    }
    
    // Test 3: Thread safety test
    std::cout << "\nTest 3: Thread safety (concurrent logging)" << std::endl;
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < 5; ++i) {
                std::string key = "thread_key";
                GameEngine::LogLimiter::error(key, "Thread {} - iteration {}", t, i);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Test 4: Generic key usage (simulating ResourceManager usage)
    std::cout << "\nTest 4: Generic key usage (like ResourceManager)" << std::endl;
    // Simulate texture_not_found with different texture names
    for (int i = 0; i < 10; ++i) {
        std::string textureName = "texture_" + std::to_string(i);
        // Use generic key, not texture-specific
        GameEngine::LogLimiter::warn("texture_not_found", "Texture not found: {}", textureName);
    }
    
    // Print statistics
    std::cout << "\n--- LogLimiter Statistics ---" << std::endl;
    auto stats = GameEngine::LogLimiter::getStats();
    for (const auto& [key, info] : stats) {
        std::cout << "Key: '" << key << "' - Count: " << info.count << std::endl;
    }
    
    std::cout << "\nTest completed. Check the output above to verify that:" << std::endl;
    std::cout << "1. Generic keys are being used (no texture names in keys)" << std::endl;
    std::cout << "2. Messages are limited to 3 occurrences per key type" << std::endl;
    std::cout << "3. A debug message appears when limit is reached" << std::endl;
    
    return 0;
}