#include <iostream>
#include <entt/entt.hpp>
#include "packages/package_manager.h"
#include "packages/package_loader.h"
#include "../packages/physics-2d/components/rigid_body.h"
#include "../packages/physics-2d/components/box_collider.h"

// Test helper macros
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "FAIL: " << message << std::endl; \
        return 1; \
    }

#define TEST_ASSERT_EQ(expected, actual, message) \
    if ((expected) != (actual)) { \
        std::cerr << "FAIL: " << message << ". Expected: " << expected << ", Actual: " << actual << std::endl; \
        return 1; \
    }

int main() {
    std::cout << "Running PackageLoader tests..." << std::endl;
    
    // Test 1: Create PackageLoader and check built-in registrations
    {
        std::cout << "\nTest 1: Built-in component and system registration..." << std::endl;
        
        GameEngine::PackageLoader loader;
        
        // Check built-in components
        TEST_ASSERT(loader.hasComponent("RigidBody"), "RigidBody component should be registered");
        TEST_ASSERT(loader.hasComponent("BoxCollider"), "BoxCollider component should be registered");
        
        // Check built-in systems
        TEST_ASSERT(loader.hasSystem("PhysicsSystem"), "PhysicsSystem should be registered");
        
        auto components = loader.getRegisteredComponents();
        auto systems = loader.getRegisteredSystems();
        
        std::cout << "Registered " << components.size() << " components and " 
                  << systems.size() << " systems" << std::endl;
        
        TEST_ASSERT(components.size() >= 2, "Should have at least 2 components registered");
        TEST_ASSERT(systems.size() >= 1, "Should have at least 1 system registered");
        
        std::cout << "PASS: Built-in registration" << std::endl;
    }
    
    // Test 2: Create components using factories
    {
        std::cout << "\nTest 2: Component factory creation..." << std::endl;
        
        GameEngine::PackageLoader loader;
        entt::registry registry;
        
        // Create an entity
        auto entity = registry.create();
        
        // Get RigidBody factory and create component
        auto rbFactory = loader.getComponentFactory("RigidBody");
        TEST_ASSERT(rbFactory != nullptr, "Should get RigidBody factory");
        
        rbFactory(registry, entity);
        TEST_ASSERT(registry.all_of<GameEngine::Physics::RigidBody>(entity), 
                   "Entity should have RigidBody component");
        
        // Get BoxCollider factory and create component
        auto bcFactory = loader.getComponentFactory("BoxCollider");
        TEST_ASSERT(bcFactory != nullptr, "Should get BoxCollider factory");
        
        bcFactory(registry, entity);
        TEST_ASSERT(registry.all_of<GameEngine::Physics::BoxCollider>(entity), 
                   "Entity should have BoxCollider component");
        
        // Verify component values
        auto& rb = registry.get<GameEngine::Physics::RigidBody>(entity);
        TEST_ASSERT_EQ(1.0f, rb.mass, "Default mass should be 1.0");
        TEST_ASSERT(rb.type == GameEngine::Physics::BodyType::Dynamic, "Default type should be Dynamic");
        
        auto& bc = registry.get<GameEngine::Physics::BoxCollider>(entity);
        TEST_ASSERT_EQ(1.0f, bc.width, "Default width should be 1.0");
        TEST_ASSERT_EQ(1.0f, bc.height, "Default height should be 1.0");
        
        std::cout << "PASS: Component factory creation" << std::endl;
    }
    
    // Test 3: Create system using factory
    {
        std::cout << "\nTest 3: System factory creation..." << std::endl;
        
        GameEngine::PackageLoader loader;
        
        auto sysFactory = loader.getSystemFactory("PhysicsSystem");
        TEST_ASSERT(sysFactory != nullptr, "Should get PhysicsSystem factory");
        
        auto system = sysFactory();
        TEST_ASSERT(system != nullptr, "Should create PhysicsSystem instance");
        
        // Initialize the system
        system->initialize();
        
        // Create a test registry
        entt::registry registry;
        
        // Update with no entities (should not crash)
        system->update(registry, 0.016f);
        
        // Shutdown
        system->shutdown();
        
        std::cout << "PASS: System factory creation" << std::endl;
    }
    
    // Test 4: Load package with PackageManager
    {
        std::cout << "\nTest 4: Loading package through PackageManager..." << std::endl;
        
        std::filesystem::path packagesDir = "../packages";
        
        if (std::filesystem::exists(packagesDir)) {
            GameEngine::PackageManager manager(packagesDir);
            
            // Scan and load physics package
            manager.scanPackages();
            bool loaded = manager.loadPackage("physics-2d");
            
            TEST_ASSERT(loaded, "Should load physics-2d package");
            
            // Check that components and systems are available
            auto& loader = manager.getPackageLoader();
            
            TEST_ASSERT(loader.hasComponent("RigidBody"), "RigidBody should be available after loading package");
            TEST_ASSERT(loader.hasComponent("BoxCollider"), "BoxCollider should be available after loading package");
            TEST_ASSERT(loader.hasSystem("PhysicsSystem"), "PhysicsSystem should be available after loading package");
            
            std::cout << "PASS: Package loading through PackageManager" << std::endl;
        } else {
            std::cout << "SKIP: Packages directory not found" << std::endl;
        }
    }
    
    // Test 5: Non-existent component/system
    {
        std::cout << "\nTest 5: Non-existent component/system handling..." << std::endl;
        
        GameEngine::PackageLoader loader;
        
        TEST_ASSERT(!loader.hasComponent("NonExistent"), "Should not have non-existent component");
        TEST_ASSERT(!loader.hasSystem("NonExistent"), "Should not have non-existent system");
        
        auto factory = loader.getComponentFactory("NonExistent");
        TEST_ASSERT(factory == nullptr, "Should return nullptr for non-existent component factory");
        
        auto sysFactory = loader.getSystemFactory("NonExistent");
        TEST_ASSERT(sysFactory == nullptr, "Should return nullptr for non-existent system factory");
        
        std::cout << "PASS: Non-existent handling" << std::endl;
    }
    
    std::cout << "\nAll PackageLoader tests passed!" << std::endl;
    return 0;
}