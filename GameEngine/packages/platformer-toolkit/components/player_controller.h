#pragma once
#include <entt/entt.hpp>

namespace GameEngine {

struct PlayerController {
    // Movement settings
    float moveSpeed = 300.0f;
    float runSpeed = 500.0f;
    float acceleration = 10.0f;
    float deceleration = 15.0f;
    
    // Jump settings
    float jumpForce = 600.0f;
    float jumpHoldMultiplier = 0.5f;
    float gravityScale = 1.0f;
    int maxJumps = 2;
    
    // Current state
    float currentSpeed = 0.0f;
    int jumpsRemaining = 2;
    bool isGrounded = false;
    bool isRunning = false;
    bool isJumping = false;
    float coyoteTime = 0.1f;  // Grace period for jumping after leaving platform
    float coyoteTimer = 0.0f;
    
    // Input buffer
    float jumpBufferTime = 0.1f;  // Accept jump input slightly before landing
    float jumpBufferTimer = 0.0f;
};

} // namespace GameEngine