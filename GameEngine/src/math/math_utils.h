#pragma once

#include <raylib.h>
#include <glm/glm.hpp>

// Utility functions for converting between GLM and Raylib math types
namespace MathUtils {
    
    // Convert GLM vec3 to Raylib Vector3
    inline Vector3 toRaylib(const glm::vec3& v) {
        return {v.x, v.y, v.z};
    }
    
    // Convert Raylib Vector3 to GLM vec3
    inline glm::vec3 fromRaylib(const Vector3& v) {
        return {v.x, v.y, v.z};
    }
    
    // Convert GLM vec2 to Raylib Vector2
    inline Vector2 toRaylib(const glm::vec2& v) {
        return {v.x, v.y};
    }
    
    // Convert Raylib Vector2 to GLM vec2
    inline glm::vec2 fromRaylib(const Vector2& v) {
        return {v.x, v.y};
    }
    
} // namespace MathUtils