#include "../src/resources/resource_manager.h"
#include "../src/utils/log_limiter.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

int main() {
    // Setup logging
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::info);
    
    std::cout << "Testing LogLimiter with generic keys in ResourceManager\n" << std::endl;
    
    // Configure LogLimiter to limit messages to 3 occurrences
    GameEngine::LogLimiter::configure(3, 60, true);
    
    // Create ResourceManager
    ResourceManager rm;
    rm.setHeadlessMode(true);
    rm.setSilentMode(false); // Enable logging
    
    std::cout << "\n--- Testing texture_not_found warnings ---" << std::endl;
    // Try to get multiple non-existent textures
    // With the old code, each would get its own limit
    // With the new code, they should all share the same limit
    for (int i = 0; i < 10; ++i) {
        std::string textureName = "nonexistent_texture_" + std::to_string(i);
        rm.getTexture(textureName);
    }
    
    std::cout << "\n--- Testing cannot_unload_texture warnings ---" << std::endl;
    // Try to unload multiple non-existent textures
    for (int i = 0; i < 10; ++i) {
        std::string textureName = "not_loaded_texture_" + std::to_string(i);
        rm.unloadTexture(textureName);
    }
    
    std::cout << "\n--- Testing texture_already_loaded messages ---" << std::endl;
    // Load a texture multiple times with different names
    for (int i = 0; i < 5; ++i) {
        std::string textureName = "test_texture_" + std::to_string(i);
        rm.loadTexture("/nonexistent/path.png", textureName);
        // Try to load again
        rm.loadTexture("/nonexistent/path.png", textureName);
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