#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <entt/entity/registry.hpp>

// Input state structure to pass keyboard/mouse state to game logic
struct InputState {
    std::unordered_map<int, bool> keys;
    std::unordered_map<int, bool> keysPressed;
    std::unordered_map<int, bool> keysReleased;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    std::unordered_map<int, bool> mouseButtons;

    bool isKeyDown(int key) const {
        auto it = keys.find(key);
        return it != keys.end() && it->second;
    }

    bool isKeyPressed(int key) const {
        auto it = keysPressed.find(key);
        return it != keysPressed.end() && it->second;
    }

    bool isKeyReleased(int key) const {
        auto it = keysReleased.find(key);
        return it != keysReleased.end() && it->second;
    }
};

// Base interface for game logic implementations
class IGameLogic {
public:
    virtual ~IGameLogic() = default;

    // Lifecycle methods
    virtual void initialize(entt::registry& registry) = 0;
    virtual void update(entt::registry& registry, float deltaTime, const InputState& input) = 0;
    virtual void fixedUpdate(entt::registry& registry, float fixedDeltaTime, const InputState& input) {}
    virtual void lateUpdate(entt::registry& registry, float deltaTime, const InputState& input) {}
    virtual void shutdown() = 0;

    // Event handling
    virtual void onEntityCreated(entt::registry& registry, entt::entity entity) {}
    virtual void onEntityDestroyed(entt::registry& registry, entt::entity entity) {}

    // Configuration
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const { return "1.0.0"; }
};

// Factory function type for creating game logic instances
using GameLogicFactory = std::unique_ptr<IGameLogic>(*)();

// Macro for easy game logic registration
#define REGISTER_GAME_LOGIC(className) \
    std::unique_ptr<IGameLogic> createGameLogic() { \
        return std::make_unique<className>(); \
    }
