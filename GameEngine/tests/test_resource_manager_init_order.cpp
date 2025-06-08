#include "../src/resources/resource_manager.h"
#include <iostream>
#include <thread>
#include <spdlog/spdlog.h>

// Test ResourceManager in different initialization scenarios
// Verify no static initialization order fiasco
// Test headless mode and graphics mode initialization

// Global ResourceManager to test static initialization
static ResourceManager g_globalResourceManager;

// Another global that uses ResourceManager in its constructor
class GlobalResourceUser {
public:
    GlobalResourceUser() {
        spdlog::info("GlobalResourceUser constructor - accessing ResourceManager");
        g_globalResourceManager.setHeadlessMode(true);
        g_globalResourceManager.setSilentMode(true);
        
        // Try to access default texture during static initialization
        auto& tex = g_globalResourceManager.getDefaultTexture();
        spdlog::info("Default texture during static init: {}x{}", tex.width, tex.height);
    }
    
    ~GlobalResourceUser() {
        spdlog::info("GlobalResourceUser destructor");
    }
};

// This will be initialized before or after g_globalResourceManager (undefined order)
static GlobalResourceUser g_globalUser;

// Function that creates ResourceManager before main
void earlyInitFunction() {
    static bool initialized = false;
    if (!initialized) {
        spdlog::info("Early init function called");
        ResourceManager rm;
        rm.setHeadlessMode(true);
        rm.getDefaultTexture();
        initialized = true;
    }
}

// Call early init before main
static int g_earlyInit = (earlyInitFunction(), 0);

int main() {
    spdlog::set_level(spdlog::level::info);
    spdlog::info("Main function started - testing initialization order safety");
    
    // Test 1: Access global ResourceManager
    spdlog::info("\nTest 1: Global ResourceManager access");
    try {
        auto& tex = g_globalResourceManager.getDefaultTexture();
        spdlog::info("Global RM default texture: {}x{}", tex.width, tex.height);
        spdlog::info("✅ Global ResourceManager works correctly");
    } catch (const std::exception& e) {
        spdlog::error("❌ Global ResourceManager failed: {}", e.what());
        return 1;
    }
    
    // Test 2: Create local ResourceManager in different modes
    spdlog::info("\nTest 2: Different initialization modes");
    
    // Headless mode
    {
        ResourceManager rm1;
        rm1.setHeadlessMode(true);
        rm1.setSilentMode(false);
        auto& tex1 = rm1.getDefaultTexture();
        spdlog::info("Headless mode texture: {}x{} (id={})", tex1.width, tex1.height, tex1.id);
        if (tex1.width != 64 || tex1.height != 64) {
            spdlog::error("❌ Headless mode texture has wrong dimensions");
            return 1;
        }
    }
    
    // Graphics mode (without RayLib initialized)
    {
        ResourceManager rm2;
        rm2.setHeadlessMode(false);
        rm2.setRayLibInitialized(false);
        rm2.setSilentMode(false);
        auto& tex2 = rm2.getDefaultTexture();
        spdlog::info("Graphics mode (no RayLib) texture: {}x{} (id={})", tex2.width, tex2.height, tex2.id);
        if (tex2.width != 64 || tex2.height != 64) {
            spdlog::error("❌ Graphics mode texture has wrong dimensions");
            return 1;
        }
    }
    
    // Test 3: Thread-local ResourceManager
    spdlog::info("\nTest 3: Thread-local ResourceManager");
    std::thread testThread([]() {
        thread_local ResourceManager tlResourceManager;
        tlResourceManager.setHeadlessMode(true);
        tlResourceManager.setSilentMode(true);
        
        auto& tex = tlResourceManager.getDefaultTexture();
        spdlog::info("Thread-local RM texture: {}x{}", tex.width, tex.height);
        
        // Load some textures
        for (int i = 0; i < 5; ++i) {
            tlResourceManager.loadTexture("test.png", "thread_tex_" + std::to_string(i));
        }
        spdlog::info("Thread-local RM loaded textures: {}", tlResourceManager.getLoadedTexturesCount());
    });
    testThread.join();
    
    // Test 4: Multiple ResourceManagers with different states
    spdlog::info("\nTest 4: Multiple ResourceManagers with different states");
    {
        ResourceManager rm1, rm2, rm3;
        
        rm1.setHeadlessMode(true);
        rm2.setHeadlessMode(false);
        rm3.setHeadlessMode(true);
        
        rm2.setRayLibInitialized(false);
        
        // All should create default textures independently
        auto& tex1 = rm1.getDefaultTexture();
        auto& tex2 = rm2.getDefaultTexture();
        auto& tex3 = rm3.getDefaultTexture();
        
        spdlog::info("RM1 texture: {}x{}", tex1.width, tex1.height);
        spdlog::info("RM2 texture: {}x{}", tex2.width, tex2.height);
        spdlog::info("RM3 texture: {}x{}", tex3.width, tex3.height);
        
        if (tex1.width != 64 || tex2.width != 64 || tex3.width != 64) {
            spdlog::error("❌ One or more ResourceManagers failed to create proper default texture");
            return 1;
        }
    }
    
    // Test 5: Lazy initialization stress test
    spdlog::info("\nTest 5: Lazy initialization stress test");
    {
        ResourceManager rm;
        rm.setHeadlessMode(true);
        rm.setSilentMode(true);
        
        // Create multiple threads that all try to get default texture at once
        const int numThreads = 10;
        std::vector<std::thread> threads;
        std::atomic<int> successCount{0};
        
        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([&rm, &successCount]() {
                auto& tex = rm.getDefaultTexture();
                if (tex.width == 64 && tex.height == 64) {
                    successCount++;
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        if (successCount != numThreads) {
            spdlog::error("❌ Lazy initialization failed in concurrent scenario");
            return 1;
        }
        spdlog::info("✅ All {} threads successfully accessed default texture", numThreads);
    }
    
    spdlog::info("\n✅ All initialization order tests passed!");
    spdlog::info("No static initialization order issues detected");
    
    return 0;
}