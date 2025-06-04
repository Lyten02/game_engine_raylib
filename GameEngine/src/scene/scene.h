#pragma once

#include <entt/entt.hpp>
#include <utility>

// Base Scene class for Entity Component System
// Scenes contain entities and manage their lifecycle
class Scene {
public:
    Scene() = default;
    virtual ~Scene() = default;
    
    // ECS Registry - public for direct access to create/manage entities
    entt::registry registry;
    
    // Lifecycle methods - override in derived classes
    virtual void onCreate() {}      // Called when scene is created
    virtual void onUpdate(float deltaTime) {}  // Called every frame
    virtual void onDestroy() {}     // Called when scene is destroyed
    
    // Helper method to create an entity with components
    template<typename T, typename... Args>
    entt::entity createEntity(Args&&... args) {
        // Create a new entity in the registry
        entt::entity entity = registry.create();
        
        // Add the component to the entity with the provided arguments
        registry.emplace<T>(entity, std::forward<Args>(args)...);
        
        return entity;
    }
};