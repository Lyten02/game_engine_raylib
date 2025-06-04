#include "engine.h"
#include <spdlog/spdlog.h>

int main() {
    spdlog::info("Game Engine starting...");
    
    // Create engine instance
    Engine engine;
    
    // Initialize engine
    if (!engine.initialize(1280, 720, "Game Engine")) {
        spdlog::error("Failed to initialize engine");
        return -1;
    }
    
    // Run the engine
    engine.run();
    
    // Shutdown
    engine.shutdown();
    
    spdlog::info("Game Engine terminated");
    return 0;
}