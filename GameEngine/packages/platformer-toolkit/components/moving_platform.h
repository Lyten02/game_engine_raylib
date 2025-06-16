#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

struct MovingPlatform {
    enum class MovementType {
        Linear,      // Move between waypoints
        Circular,    // Circular motion
        Sine         // Sine wave motion
    };
    
    MovementType type = MovementType::Linear;
    std::vector<glm::vec3> waypoints;
    int currentWaypoint = 0;
    
    float speed = 100.0f;
    float waitTime = 1.0f;  // Time to wait at each waypoint
    float currentWaitTime = 0.0f;
    
    // For circular/sine movement
    glm::vec3 center;
    float radius = 100.0f;
    float angle = 0.0f;
    
    // Platform behavior
    bool loop = true;  // Loop through waypoints
    bool pingPong = false;  // Go back and forth
    bool movingForward = true;
    
    // Passengers (entities on the platform)
    std::vector<entt::entity> passengers;
};

} // namespace GameEngine