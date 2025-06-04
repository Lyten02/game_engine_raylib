#pragma once

#include <raylib.h>
#include <nlohmann/json.hpp>

// Transform component for Entity Component System
// Stores position, rotation, and scale data for entities
// Named TransformComponent to avoid conflict with Raylib's Transform
struct TransformComponent {
    Vector3 position{0.0f, 0.0f, 0.0f};  // Position in 3D space
    Vector3 rotation{0.0f, 0.0f, 0.0f};  // Rotation in degrees (pitch, yaw, roll)
    Vector3 scale{1.0f, 1.0f, 1.0f};     // Scale factors for each axis
    
    nlohmann::json to_json() const {
        return {
            {"position", {position.x, position.y, position.z}},
            {"rotation", {rotation.x, rotation.y, rotation.z}},
            {"scale", {scale.x, scale.y, scale.z}}
        };
    }
    
    void from_json(const nlohmann::json& j) {
        if (j.contains("position")) {
            const auto& pos = j["position"];
            position = {pos[0], pos[1], pos[2]};
        }
        if (j.contains("rotation")) {
            const auto& rot = j["rotation"];
            rotation = {rot[0], rot[1], rot[2]};
        }
        if (j.contains("scale")) {
            const auto& s = j["scale"];
            scale = {s[0], s[1], s[2]};
        }
    }
};