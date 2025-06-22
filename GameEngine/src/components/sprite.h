#pragma once

#include <raylib.h>
#include <string>
#include <nlohmann/json.hpp>

struct Sprite {
    Texture2D* texture = nullptr;
    Rectangle sourceRect{0.0f, 0.0f, 0.0f, 0.0f};
    Color tint = WHITE;
    std::string texturePath; // For serialization

    nlohmann::json to_json() const {
        return {
            {"texture", texturePath},
            {"source", {sourceRect.x, sourceRect.y, sourceRect.width, sourceRect.height}},
            {"tint", {tint.r, tint.g, tint.b, tint.a}}
        };
    }

    void from_json(const nlohmann::json& j) {
        if (j.contains("texture")) {
            texturePath = j["texture"];
        }
        if (j.contains("source")) {
            const auto& src = j["source"];
            sourceRect = {src[0], src[1], src[2], src[3]};
        }
        if (j.contains("tint")) {
            const auto& t = j["tint"];
            tint = {
                static_cast<unsigned char>(t[0]),
                static_cast<unsigned char>(t[1]),
                static_cast<unsigned char>(t[2]),
                static_cast<unsigned char>(t[3])
            };
        }
    }
};
