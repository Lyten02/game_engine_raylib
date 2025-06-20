#include <iostream>
#include <vector>
#include <thread>
#include "systems/render_system.h"
#include "components/transform.h"
#include "components/sprite.h"
#include "resources/resource_manager.h"
#include <entt/entt.hpp>
#include "raylib.h"

class TestRenderSystem : public RenderSystem {
public:
    TestRenderSystem() {
        setTestMode(true);
    }
    
    // Override camera methods to avoid RayLib calls in headless mode
    void beginCamera() override {
        // No-op in test mode
    }
    
    void endCamera() override {
        // No-op in test mode
    }
    
    bool testEntityDeletionDuringUpdate() {
        // Create registry
        entt::registry registry;
        
        // Create ResourceManager
        ResourceManager resourceManager;
        resourceManager.setSilentMode(true);
        resourceManager.setHeadlessMode(true);
        resourceManager.setRayLibInitialized(false);
        
        // Create test entities
        std::vector<entt::entity> entities;
        for (int i = 0; i < 10; i++) {
            auto entity = registry.create();
            entities.push_back(entity);
            
            // Add components
            registry.emplace<TransformComponent>(entity);
            auto& sprite = registry.emplace<Sprite>(entity);
            sprite.texture = resourceManager.getTexture("test_texture");
        }
        
        // Test 1: Call actual update method while deleting entities
        try {
            // Set up camera
            Camera2D camera = {0};
            setCamera2D(camera);
            
            // Call update - should not crash
            update(registry);
            
            // Delete some entities
            registry.destroy(entities[2]);
            registry.destroy(entities[5]);
            registry.destroy(entities[7]);
            
            // Call update again - should handle deleted entities
            update(registry);
            
            std::cout << "PASS: update() handles deleted entities correctly" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "FAIL: Exception in update(): " << e.what() << std::endl;
            return false;
        }
        
        return true;
    }
    
    bool testConcurrentEntityDeletion() {
        // Test concurrent deletion scenario
        entt::registry registry;
        ResourceManager resourceManager;
        resourceManager.setSilentMode(true);
        resourceManager.setHeadlessMode(true);
        resourceManager.setRayLibInitialized(false);
        
        // Create many entities
        std::vector<entt::entity> entities;
        for (int i = 0; i < 100; i++) {
            auto entity = registry.create();
            entities.push_back(entity);
            registry.emplace<TransformComponent>(entity);
            auto& sprite = registry.emplace<Sprite>(entity);
            sprite.texture = resourceManager.getTexture("test_texture");
        }
        
        try {
            // Simulate concurrent modification
            std::thread deletionThread([&registry, &entities]() {
                for (int i = 10; i < 20; i++) {
                    if (registry.valid(entities[i])) {
                        registry.destroy(entities[i]);
                    }
                }
            });
            
            // Try to render while deletion happens
            Camera2D camera = {0};
            setCamera2D(camera);
            update(registry);
            
            deletionThread.join();
            
            // Render again after deletion
            update(registry);
            
            std::cout << "PASS: Concurrent deletion handled" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "FAIL: Exception during concurrent deletion: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool testInvalidEntityAccess() {
        entt::registry registry;
        
        // Create an entity and immediately destroy it
        auto entity = registry.create();
        registry.emplace<TransformComponent>(entity);
        registry.emplace<Sprite>(entity);
        registry.destroy(entity);
        
        // Create valid entities
        for (int i = 0; i < 5; i++) {
            auto e = registry.create();
            registry.emplace<TransformComponent>(e);
            registry.emplace<Sprite>(e);
        }
        
        try {
            Camera2D camera = {0};
            setCamera2D(camera);
            update(registry);
            
            std::cout << "PASS: Invalid entity access handled" << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "FAIL: Exception on invalid entity: " << e.what() << std::endl;
            return false;
        }
    }
};

int main() {
    std::cout << "Running RenderSystem null pointer dereference test..." << std::endl;
    
    // Initialize minimal RayLib
    SetTraceLogLevel(LOG_NONE);
    
    TestRenderSystem testSystem;
    
    // Test 1: Entity deletion during update
    if (!testSystem.testEntityDeletionDuringUpdate()) {
        return 1;
    }
    
    // Test 2: Invalid entity access
    if (!testSystem.testInvalidEntityAccess()) {
        return 1;
    }
    
    // Test 3: Concurrent entity deletion (most likely to cause issues)
    std::cout << "\nTesting concurrent deletion scenario..." << std::endl;
    if (!testSystem.testConcurrentEntityDeletion()) {
        return 1;
    }
    
    std::cout << "\nAll tests passed!" << std::endl;
    return 0;
}