#pragma once

#include <raylib.h>

struct Sprite {
    Texture2D* texture = nullptr;
    Rectangle sourceRect{0.0f, 0.0f, 0.0f, 0.0f};
    Color tint = WHITE;
};