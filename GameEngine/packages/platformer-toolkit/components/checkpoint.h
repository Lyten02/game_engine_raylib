#pragma once
#include <glm/glm.hpp>

namespace GameEngine {

struct Checkpoint {
    bool isActivated = false;
    glm::vec3 respawnPosition;
    
    // Visual feedback
    float animationTime = 0.0f;
    bool showParticles = false;
    
    // Optional: checkpoint order for sequential levels
    int checkpointId = 0;
};

} // namespace GameEngine