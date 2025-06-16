#pragma once

#include <raylib.h>

struct CameraComponent {
    Vector2 target = {0.0f, 0.0f};
    Vector2 offset = {640.0f, 360.0f};  // Default to center of 1280x720 screen
    float rotation = 0.0f;
    float zoom = 1.0f;
    bool active = true;
};