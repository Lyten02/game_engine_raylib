#pragma once

#include <string>
#include <typeindex>

// Component type identifiers for optional components
namespace ComponentTypes {
    // Well-known component type names that plugins can register
    constexpr const char* TRANSFORM = "Transform";
    constexpr const char* SPRITE = "Sprite";
    constexpr const char* CAMERA = "Camera";
    
    // Component categories
    constexpr const char* CATEGORY_RENDERING = "Rendering";
    constexpr const char* CATEGORY_TRANSFORM = "Transform";
    constexpr const char* CATEGORY_PHYSICS = "Physics";
    constexpr const char* CATEGORY_LOGIC = "Logic";
}

// Base interface for component metadata
struct ComponentMetadata {
    std::string name;
    std::string category;
    std::type_index typeIndex;
    
    ComponentMetadata(const std::string& n, const std::string& c, std::type_index t)
        : name(n), category(c), typeIndex(t) {}
};