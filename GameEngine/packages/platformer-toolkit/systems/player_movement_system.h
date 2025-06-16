#pragma once
#include "../../../src/systems/system.h"
#include "../components/player_controller.h"
#include "../../../src/components/transform.h"
#include "../../physics-2d/components/rigidbody.h"
#include <raylib.h>
#include <algorithm>

namespace GameEngine {

class PlayerMovementSystem : public ISystem {
public:
    void update(entt::registry& registry, float deltaTime) override {
        auto view = registry.view<PlayerController, TransformComponent, RigidBody>();
        
        for (auto entity : view) {
            auto& controller = view.get<PlayerController>(entity);
            auto& transform = view.get<TransformComponent>(entity);
            auto& rb = view.get<RigidBody>(entity);
            
            // Update timers
            if (controller.coyoteTimer > 0) {
                controller.coyoteTimer -= deltaTime;
            }
            if (controller.jumpBufferTimer > 0) {
                controller.jumpBufferTimer -= deltaTime;
            }
            
            // Horizontal movement
            float targetSpeed = 0.0f;
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
                targetSpeed = -controller.moveSpeed;
                if (controller.isRunning) targetSpeed = -controller.runSpeed;
            }
            else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
                targetSpeed = controller.moveSpeed;
                if (controller.isRunning) targetSpeed = controller.runSpeed;
            }
            
            // Apply acceleration/deceleration
            if (targetSpeed != 0) {
                controller.currentSpeed = std::lerp(
                    controller.currentSpeed, 
                    targetSpeed, 
                    controller.acceleration * deltaTime
                );
            } else {
                controller.currentSpeed = std::lerp(
                    controller.currentSpeed, 
                    0.0f, 
                    controller.deceleration * deltaTime
                );
            }
            
            // Apply horizontal velocity
            rb.velocity.x = controller.currentSpeed;
            
            // Check for running
            controller.isRunning = IsKeyDown(KEY_LEFT_SHIFT);
            
            // Jump input
            if (IsKeyPressed(KEY_SPACE)) {
                controller.jumpBufferTimer = controller.jumpBufferTime;
            }
            
            // Jump logic
            bool canJump = controller.isGrounded || 
                          (controller.coyoteTimer > 0 && controller.jumpsRemaining == controller.maxJumps) ||
                          (controller.jumpsRemaining > 0 && controller.jumpsRemaining < controller.maxJumps);
            
            if (controller.jumpBufferTimer > 0 && canJump) {
                // Perform jump
                rb.velocity.y = -controller.jumpForce; // Negative because Y goes down
                controller.jumpsRemaining--;
                controller.isJumping = true;
                controller.jumpBufferTimer = 0;
                
                // Reset coyote time
                controller.coyoteTimer = 0;
            }
            
            // Variable jump height
            if (controller.isJumping && !IsKeyDown(KEY_SPACE) && rb.velocity.y < 0) {
                rb.velocity.y *= controller.jumpHoldMultiplier;
                controller.isJumping = false;
            }
            
            // Apply gravity scale
            rb.gravityScale = controller.gravityScale;
            
            // Ground check would be done by physics system
            // For now, simple check
            if (transform.position.y >= 400) { // Assuming ground at y=400
                controller.isGrounded = true;
                controller.jumpsRemaining = controller.maxJumps;
                controller.coyoteTimer = controller.coyoteTime;
            } else {
                controller.isGrounded = false;
            }
        }
    }
    
private:
    // Simple lerp function
    float std::lerp(float a, float b, float t) {
        return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
    }
};

} // namespace GameEngine