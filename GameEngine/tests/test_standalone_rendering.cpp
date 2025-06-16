#include <iostream>
#include <cassert>
#include <cmath>
#include <entt/entt.hpp>
#include <raylib.h>

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition) do { \
    tests_run++; \
    if (!(condition)) { \
        std::cerr << "❌ Test failed at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "   Condition: " << #condition << std::endl; \
    } else { \
        tests_passed++; \
    } \
} while(0)

#define TEST_ASSERT_FLOAT_EQ(a, b) TEST_ASSERT(std::abs((a) - (b)) < 0.001f)

// Mock structures to test rendering logic
struct TransformComponent {
    Vector3 position{0.0f, 0.0f, 0.0f};
    Vector3 rotation{0.0f, 0.0f, 0.0f};
    Vector3 scale{1.0f, 1.0f, 1.0f};
};

struct Sprite {
    Texture2D* texture = nullptr;
    Rectangle sourceRect{0.0f, 0.0f, 0.0f, 0.0f};
    Color tint = WHITE;
    std::string texturePath;
};

// Function to test - this simulates the rendering logic
bool shouldRenderColoredRectangle(const Sprite& sprite) {
    // Should render colored rectangle when:
    // 1. No texture is present
    // 2. Source rect has valid dimensions OR is zero (which means use scale from transform)
    return sprite.texture == nullptr;
}

Rectangle calculateSpriteRectangle(const TransformComponent& transform, const Sprite& sprite) {
    // If sourceRect is zero, use transform scale
    if (sprite.sourceRect.width == 0 || sprite.sourceRect.height == 0) {
        return Rectangle{
            transform.position.x - transform.scale.x / 2,
            transform.position.y - transform.scale.y / 2,
            transform.scale.x,
            transform.scale.y
        };
    }
    // Otherwise use sourceRect dimensions
    return Rectangle{
        transform.position.x - sprite.sourceRect.width / 2,
        transform.position.y - sprite.sourceRect.height / 2,
        sprite.sourceRect.width,
        sprite.sourceRect.height
    };
}

void test_should_render_colored_rectangle_without_texture() {
    std::cout << "Test: Should render colored rectangle without texture... ";
    entt::registry registry;
    // Create entity with sprite but no texture
    auto entity = registry.create();
    auto& transform = registry.emplace<TransformComponent>(entity);
    auto& sprite = registry.emplace<Sprite>(entity);
    
    transform.position = {100, 100, 0};
    transform.scale = {50, 50, 1};
    sprite.tint = GREEN;
    sprite.texture = nullptr;
    
    // Test that it should render as colored rectangle
    TEST_ASSERT(shouldRenderColoredRectangle(sprite));
    std::cout << "✓" << std::endl;
}

void test_should_not_render_colored_rectangle_with_texture() {
    std::cout << "Test: Should not render colored rectangle with texture... ";
    entt::registry registry;
    // Create entity with sprite and texture
    auto entity = registry.create();
    auto& sprite = registry.emplace<Sprite>(entity);
    
    // Simulate having a texture
    Texture2D dummyTexture;
    dummyTexture.id = 1; // Non-zero ID indicates valid texture
    sprite.texture = &dummyTexture;
    
    // Test that it should NOT render as colored rectangle
    TEST_ASSERT(!shouldRenderColoredRectangle(sprite));
    std::cout << "✓" << std::endl;
}

void test_calculate_rectangle_from_transform_scale() {
    std::cout << "Test: Calculate rectangle from transform scale... ";
    TransformComponent transform;
    transform.position = {640, 360, 0};
    transform.scale = {100, 80, 1};
    
    Sprite sprite;
    sprite.sourceRect = {0, 0, 0, 0}; // Zero dimensions
    
    Rectangle rect = calculateSpriteRectangle(transform, sprite);
    
    // Should use transform scale and center around position
    TEST_ASSERT_FLOAT_EQ(rect.x, 640 - 50); // position.x - scale.x/2
    TEST_ASSERT_FLOAT_EQ(rect.y, 360 - 40); // position.y - scale.y/2
    TEST_ASSERT_FLOAT_EQ(rect.width, 100);
    TEST_ASSERT_FLOAT_EQ(rect.height, 80);
    std::cout << "✓" << std::endl;
}

void test_calculate_rectangle_from_source_rect() {
    std::cout << "Test: Calculate rectangle from source rect... ";
    TransformComponent transform;
    transform.position = {200, 150, 0};
    transform.scale = {100, 100, 1}; // Will be ignored
    
    Sprite sprite;
    sprite.sourceRect = {0, 0, 32, 32}; // Has dimensions
    
    Rectangle rect = calculateSpriteRectangle(transform, sprite);
    
    // Should use source rect dimensions
    TEST_ASSERT_FLOAT_EQ(rect.x, 200 - 16); // position.x - sourceRect.width/2
    TEST_ASSERT_FLOAT_EQ(rect.y, 150 - 16); // position.y - sourceRect.height/2
    TEST_ASSERT_FLOAT_EQ(rect.width, 32);
    TEST_ASSERT_FLOAT_EQ(rect.height, 32);
    std::cout << "✓" << std::endl;
}

void test_multiple_entities_with_mixed_render_types() {
    std::cout << "Test: Multiple entities with mixed render types... ";
    entt::registry registry;
    // Entity 1: With texture
    auto entity1 = registry.create();
    auto& sprite1 = registry.emplace<Sprite>(entity1);
    Texture2D texture1;
    texture1.id = 1;
    sprite1.texture = &texture1;
    
    // Entity 2: Without texture (should render as colored rect)
    auto entity2 = registry.create();
    auto& sprite2 = registry.emplace<Sprite>(entity2);
    sprite2.texture = nullptr;
    sprite2.tint = RED;
    
    // Entity 3: Without texture, different color
    auto entity3 = registry.create();
    auto& sprite3 = registry.emplace<Sprite>(entity3);
    sprite3.texture = nullptr;
    sprite3.tint = BLUE;
    
    // Count entities that should render as colored rectangles
    int coloredRectCount = 0;
    auto view = registry.view<Sprite>();
    for (auto entity : view) {
        const auto& sprite = view.get<Sprite>(entity);
        if (shouldRenderColoredRectangle(sprite)) {
            coloredRectCount++;
        }
    }
    
    TEST_ASSERT(coloredRectCount == 2);
    std::cout << "✓" << std::endl;
}

int main() {
    std::cout << "\n=== Running Standalone Rendering Tests ===\n" << std::endl;
    
    test_should_render_colored_rectangle_without_texture();
    test_should_not_render_colored_rectangle_with_texture();
    test_calculate_rectangle_from_transform_scale();
    test_calculate_rectangle_from_source_rect();
    test_multiple_entities_with_mixed_render_types();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Tests run: " << tests_run << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << (tests_run - tests_passed) << std::endl;
    
    return (tests_run == tests_passed) ? 0 : 1;
}