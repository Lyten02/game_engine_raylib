#pragma once

#include <raylib.h>

// Sprite component for Entity Component System
// Contains texture data and rendering properties for 2D sprites
struct Sprite {
    Texture2D texture{};                              // The texture to render
    Rectangle source{0.0f, 0.0f, 0.0f, 0.0f};        // Source rectangle from texture (for sprite sheets)
    Color tint = WHITE;                               // Color tint to apply when rendering
};