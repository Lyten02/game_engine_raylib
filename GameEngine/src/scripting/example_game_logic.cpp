#include "game_logic_interface.h"
#include "../components/transform.h"
#include <spdlog/spdlog.h>
#include <raylib.h>

// Example game logic implementation
class ExampleGameLogic : public IGameLogic {
private:
    float timeElapsed = 0.0f;

public:
    void initialize(entt::registry& registry) override {
        spdlog::info("ExampleGameLogic initialized");
    }
    
    void update(entt::registry& registry, float deltaTime) override {
        timeElapsed += deltaTime;
        
        // Example: Rotate all entities with transform
        auto view = registry.view<TransformComponent>();
        for (auto entity : view) {
            auto& transform = view.get<TransformComponent>(entity);
            
            // Rotate around Y axis
            transform.rotation.y += 45.0f * deltaTime;
            
            // Simple bobbing motion
            transform.position.y = transform.position.y + std::sin(timeElapsed * 2.0f) * deltaTime * 0.5f;
        }
    }
    
    void onEntityCreated(entt::registry& registry, entt::entity entity) override {
        spdlog::debug("Entity created: {}", static_cast<uint32_t>(entity));
    }
    
    void onEntityDestroyed(entt::registry& registry, entt::entity entity) override {
        spdlog::debug("Entity destroyed: {}", static_cast<uint32_t>(entity));
    }
    
    void shutdown() override {
        spdlog::info("ExampleGameLogic shutdown");
    }
    
    std::string getName() const override {
        return "ExampleGameLogic";
    }
};

// Register this game logic
REGISTER_GAME_LOGIC(ExampleGameLogic)