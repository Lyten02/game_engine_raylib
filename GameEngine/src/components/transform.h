#pragma once

#include <raylib.h>

// Transform component for Entity Component System
// Stores position, rotation, and scale data for entities
// Named TransformComponent to avoid conflict with Raylib's Transform
struct TransformComponent {
    Vector3 position{0.0f, 0.0f, 0.0f};  // Position in 3D space
    Vector3 rotation{0.0f, 0.0f, 0.0f};  // Rotation in degrees (pitch, yaw, roll)
    Vector3 scale{1.0f, 1.0f, 1.0f};     // Scale factors for each axis
};