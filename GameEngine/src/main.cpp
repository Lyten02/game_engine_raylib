#include "engine.h"
#include <spdlog/spdlog.h>

#ifdef __APPLE__
    #include <pthread.h>
#endif

int main() {
    spdlog::info("Game Engine starting...");
    
    #ifdef __APPLE__
        // Set thread priority for macOS to prevent throttling
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    #endif
    
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