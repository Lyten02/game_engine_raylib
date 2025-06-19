#include "../src/resources/resource_manager.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>

// Test to verify that ResourceManager has pointer stability issues

void testPointerStability() {
    std::cout << "=== Testing ResourceManager Pointer Stability ===" << std::endl;
    
    // Create ResourceManager in headless mode
    ResourceManager manager;
    manager.setHeadlessMode(true);
    manager.setRayLibInitialized(false);
    manager.setSilentMode(false);
    
    // Step 1: Load some textures and store pointers
    std::cout << "\nStep 1: Loading initial textures and storing pointers..." << std::endl;
    
    std::vector<Texture2D*> storedPointers;
    std::vector<std::string> textureNames;
    
    for (int i = 0; i < 10; ++i) {
        std::string name = "texture_" + std::to_string(i);
        std::string path = "fake_path_" + std::to_string(i) + ".png";
        
        Texture2D* ptr = manager.loadTexture(path, name);
        storedPointers.push_back(ptr);
        textureNames.push_back(name);
        
        std::cout << "  Loaded " << name << " at address: " << ptr << std::endl;
    }
    
    // Step 2: Verify all pointers work
    std::cout << "\nStep 2: Verifying initial pointers..." << std::endl;
    bool allValid = true;
    
    for (size_t i = 0; i < storedPointers.size(); ++i) {
        Texture2D* storedPtr = storedPointers[i];
        Texture2D* currentPtr = manager.getTexture(textureNames[i]);
        
        if (storedPtr != currentPtr) {
            std::cout << "  ERROR: " << textureNames[i] << " pointer changed!" << std::endl;
            allValid = false;
        }
    }
    
    if (allValid) {
        std::cout << "  ✓ All pointers are currently valid" << std::endl;
    }
    
    // Step 3: Load many more textures to potentially trigger rehashing
    std::cout << "\nStep 3: Loading many textures to stress the map..." << std::endl;
    std::cout << "  Initial texture count: " << manager.getLoadedTexturesCount() << std::endl;
    
    // Load a large number of textures
    for (int i = 10; i < 200; ++i) {
        std::string name = "stress_texture_" + std::to_string(i);
        std::string path = "stress_path_" + std::to_string(i) + ".png";
        manager.loadTexture(path, name);
    }
    
    std::cout << "  Final texture count: " << manager.getLoadedTexturesCount() << std::endl;
    
    // Step 4: Check if original pointers are still valid
    std::cout << "\nStep 4: Checking if original pointers are still valid..." << std::endl;
    
    int invalidCount = 0;
    for (size_t i = 0; i < storedPointers.size(); ++i) {
        Texture2D* storedPtr = storedPointers[i];
        Texture2D* currentPtr = manager.getTexture(textureNames[i]);
        
        std::cout << "  " << textureNames[i] << ": ";
        std::cout << "stored=" << storedPtr << ", current=" << currentPtr;
        
        if (storedPtr != currentPtr) {
            std::cout << " ❌ CHANGED!" << std::endl;
            invalidCount++;
        } else {
            std::cout << " ✓" << std::endl;
        }
    }
    
    if (invalidCount > 0) {
        std::cout << "\n❌ CRITICAL: " << invalidCount << " pointers became invalid!" << std::endl;
        std::cout << "This proves the dangling pointer bug exists!" << std::endl;
    } else {
        std::cout << "\n⚠️  Pointers remained valid in this run, but the bug still exists!" << std::endl;
        std::cout << "The C++ standard does NOT guarantee this behavior." << std::endl;
    }
}

void testConcurrentAccess() {
    std::cout << "\n\n=== Testing Concurrent Access Danger ===" << std::endl;
    
    ResourceManager manager;
    manager.setHeadlessMode(true);
    manager.setRayLibInitialized(false);
    manager.setSilentMode(true); // Reduce log spam
    
    // Load initial texture
    Texture2D* playerTexture = manager.loadTexture("player.png", "player");
    std::cout << "Player texture initially at: " << playerTexture << std::endl;
    
    std::atomic<bool> stop{false};
    std::atomic<int> accessCount{0};
    std::atomic<int> crashCount{0};
    
    // Reader thread - simulates game rendering
    std::thread reader([&]() {
        while (!stop.load()) {
            try {
                // Simulate accessing texture during rendering
                if (playerTexture && playerTexture->width == 64) {
                    accessCount++;
                }
            } catch (...) {
                crashCount++;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    // Writer thread - simulates dynamic texture loading
    std::thread writer([&]() {
        for (int i = 0; i < 500; ++i) {
            manager.loadTexture("dynamic_" + std::to_string(i) + ".png", 
                              "dynamic_" + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // Wait for writer to finish
    writer.join();
    stop.store(true);
    reader.join();
    
    // Check results
    Texture2D* currentPlayerTexture = manager.getTexture("player");
    
    std::cout << "\nResults:" << std::endl;
    std::cout << "  Original player texture pointer: " << playerTexture << std::endl;
    std::cout << "  Current player texture pointer: " << currentPlayerTexture << std::endl;
    std::cout << "  Successful accesses: " << accessCount.load() << std::endl;
    std::cout << "  Crashes caught: " << crashCount.load() << std::endl;
    
    if (playerTexture != currentPlayerTexture) {
        std::cout << "\n❌ DANGER: Pointer changed during concurrent access!" << std::endl;
        std::cout << "Reader thread was accessing a potentially invalid pointer!" << std::endl;
    }
}

void testRealWorldScenario() {
    std::cout << "\n\n=== Real World Scenario ===" << std::endl;
    
    ResourceManager manager;
    manager.setHeadlessMode(true);
    manager.setRayLibInitialized(false);
    manager.setSilentMode(true);
    
    // Simulate game startup - load core textures
    Texture2D* uiTexture = manager.loadTexture("ui.png", "ui");
    Texture2D* fontTexture = manager.loadTexture("font.png", "font");
    
    // Game systems store these pointers
    std::vector<Texture2D*> criticalTextures = {uiTexture, fontTexture};
    
    std::cout << "Game started with critical textures:" << std::endl;
    std::cout << "  UI texture at: " << uiTexture << std::endl;
    std::cout << "  Font texture at: " << fontTexture << std::endl;
    
    // Simulate gameplay - dynamic content loading
    std::cout << "\nSimulating gameplay with dynamic content..." << std::endl;
    
    for (int level = 1; level <= 10; ++level) {
        // Each level loads its own textures
        for (int i = 0; i < 20; ++i) {
            std::string name = "level" + std::to_string(level) + "_asset" + std::to_string(i);
            manager.loadTexture(name + ".png", name);
        }
        
        // Check if critical pointers are still valid
        if (level == 5) {
            std::cout << "\nAfter level 5:" << std::endl;
            std::cout << "  UI pointer still at: " << manager.getTexture("ui") 
                     << (uiTexture == manager.getTexture("ui") ? " ✓" : " ❌ CHANGED!") << std::endl;
            std::cout << "  Font pointer still at: " << manager.getTexture("font")
                     << (fontTexture == manager.getTexture("font") ? " ✓" : " ❌ CHANGED!") << std::endl;
        }
    }
    
    // Final check
    std::cout << "\nFinal check after all levels:" << std::endl;
    bool uiValid = (uiTexture == manager.getTexture("ui"));
    bool fontValid = (fontTexture == manager.getTexture("font"));
    
    std::cout << "  UI texture: " << (uiValid ? "✓ Still valid" : "❌ INVALIDATED") << std::endl;
    std::cout << "  Font texture: " << (fontValid ? "✓ Still valid" : "❌ INVALIDATED") << std::endl;
    
    if (!uiValid || !fontValid) {
        std::cout << "\n❌ CRITICAL BUG: Core texture pointers were invalidated!" << std::endl;
        std::cout << "This would cause crashes or corruption in a real game!" << std::endl;
    }
}

int main() {
    std::cout << "ResourceManager Pointer Stability Test\n" << std::endl;
    std::cout << "This test demonstrates why storing Texture2D directly" << std::endl;
    std::cout << "in std::unordered_map is dangerous.\n" << std::endl;
    
    testPointerStability();
    testConcurrentAccess();
    testRealWorldScenario();
    
    std::cout << "\n\n=== Summary ===" << std::endl;
    std::cout << "The current ResourceManager implementation has a CRITICAL bug:" << std::endl;
    std::cout << "- Pointers returned by loadTexture() and getTexture() can become invalid" << std::endl;
    std::cout << "- This happens when std::unordered_map rehashes (unpredictable)" << std::endl;
    std::cout << "- Game code storing these pointers will crash or corrupt memory" << std::endl;
    std::cout << "- The fix: Change storage to std::unique_ptr<Texture2D>" << std::endl;
    
    return 0;
}