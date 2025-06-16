#pragma once
#include "../../../src/systems/system.h"
#include "../components/moving_platform.h"
#include "../../../src/components/transform.h"
#include <glm/glm.hpp>
#include <cmath>

namespace GameEngine {

class PlatformSystem : public ISystem {
public:
    void update(entt::registry& registry, float deltaTime) override {
        auto view = registry.view<MovingPlatform, TransformComponent>();
        
        for (auto entity : view) {
            auto& platform = view.get<MovingPlatform>(entity);
            auto& transform = view.get<TransformComponent>(entity);
            
            // Update wait timer
            if (platform.currentWaitTime > 0) {
                platform.currentWaitTime -= deltaTime;
                continue;
            }
            
            switch (platform.type) {
                case MovingPlatform::MovementType::Linear:
                    updateLinearMovement(platform, transform, deltaTime);
                    break;
                    
                case MovingPlatform::MovementType::Circular:
                    updateCircularMovement(platform, transform, deltaTime);
                    break;
                    
                case MovingPlatform::MovementType::Sine:
                    updateSineMovement(platform, transform, deltaTime);
                    break;
            }
            
            // Move passengers with the platform
            glm::vec3 platformDelta = transform.position - lastPosition[entity];
            for (auto passenger : platform.passengers) {
                if (registry.valid(passenger)) {
                    auto* passengerTransform = registry.try_get<TransformComponent>(passenger);
                    if (passengerTransform) {
                        passengerTransform->position += platformDelta;
                    }
                }
            }
            
            lastPosition[entity] = transform.position;
        }
    }
    
private:
    std::unordered_map<entt::entity, glm::vec3> lastPosition;
    
    void updateLinearMovement(MovingPlatform& platform, TransformComponent& transform, float deltaTime) {
        if (platform.waypoints.size() < 2) return;
        
        glm::vec3 target = platform.waypoints[platform.currentWaypoint];
        glm::vec3 direction = glm::normalize(target - transform.position);
        
        // Move towards target
        transform.position += direction * platform.speed * deltaTime;
        
        // Check if reached waypoint
        if (glm::distance(transform.position, target) < 5.0f) {
            transform.position = target;
            platform.currentWaitTime = platform.waitTime;
            
            // Next waypoint
            if (platform.pingPong) {
                if (platform.movingForward) {
                    platform.currentWaypoint++;
                    if (platform.currentWaypoint >= platform.waypoints.size()) {
                        platform.currentWaypoint = platform.waypoints.size() - 2;
                        platform.movingForward = false;
                    }
                } else {
                    platform.currentWaypoint--;
                    if (platform.currentWaypoint < 0) {
                        platform.currentWaypoint = 1;
                        platform.movingForward = true;
                    }
                }
            } else if (platform.loop) {
                platform.currentWaypoint = (platform.currentWaypoint + 1) % platform.waypoints.size();
            }
        }
    }
    
    void updateCircularMovement(MovingPlatform& platform, TransformComponent& transform, float deltaTime) {
        platform.angle += platform.speed * deltaTime * 0.01f;
        
        transform.position.x = platform.center.x + std::cos(platform.angle) * platform.radius;
        transform.position.y = platform.center.y + std::sin(platform.angle) * platform.radius;
    }
    
    void updateSineMovement(MovingPlatform& platform, TransformComponent& transform, float deltaTime) {
        platform.angle += platform.speed * deltaTime * 0.01f;
        
        if (platform.waypoints.size() >= 2) {
            glm::vec3 start = platform.waypoints[0];
            glm::vec3 end = platform.waypoints[1];
            
            float t = (std::sin(platform.angle) + 1.0f) * 0.5f; // 0 to 1
            transform.position = glm::mix(start, end, t);
        }
    }
};

} // namespace GameEngine