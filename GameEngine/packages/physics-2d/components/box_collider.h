#pragma once

namespace GameEngine::Physics {

struct BoxCollider {
    float width = 1.0f;
    float height = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    
    // Physics properties
    float friction = 0.3f;
    float restitution = 0.0f;
    float density = 1.0f;
    
    // Collision filtering
    unsigned short categoryBits = 0x0001;
    unsigned short maskBits = 0xFFFF;
    
    bool isSensor = false;
};

} // namespace GameEngine::Physics