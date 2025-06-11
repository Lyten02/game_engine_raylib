#include "demo_game.h"
#include "components/transform.h"
#include "components/sprite.h"
#include "components/physics.h"
#include "components/player_controller.h"
#include "utils/config.h"
#include <raylib.h>
#include <spdlog/spdlog.h>
#include <random>

void DemoGame::onCreate() {
    spdlog::info("DemoGame::onCreate - Initializing demo game scene");
    
    // Load settings from config
    loadGameSettings();
    
    // Create game world
    createGround();
    createWalls();
    createPlayer();
    
    // Spawn initial boxes
    for (int i = 0; i < 5; ++i) {
        float x = -200.0f + (i * 100.0f);
        float y = 200.0f + (i * 50.0f);
        spawnBox(x, y);
    }
    
    spdlog::info("Demo game scene created successfully");
}

void DemoGame::onUpdate(float deltaTime) {
    handleInput();
    
    // Auto-spawn boxes
    if (autoSpawnBoxes && boxes.size() < static_cast<size_t>(maxBoxes)) {
        spawnTimer += deltaTime;
        if (spawnTimer >= spawnInterval) {
            spawnTimer = 0.0f;
            
            // Random spawn position
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_real_distribution<float> dis(-300.0f, 300.0f);
            
            spawnBox(dis(gen), 400.0f);
        }
    }
    
    // Remove boxes that fall too far
    auto it = boxes.begin();
    while (it != boxes.end()) {
        if (registry.valid(*it)) {
            auto& transform = registry.get<TransformComponent>(*it);
            if (transform.position.y > 800.0f) {
                registry.destroy(*it);
                it = boxes.erase(it);
                spdlog::debug("Removed box that fell out of bounds");
            } else {
                ++it;
            }
        } else {
            it = boxes.erase(it);
        }
    }
    
    updateUI();
}

void DemoGame::onDestroy() {
    spdlog::info("DemoGame::onDestroy - Cleaning up demo game scene");
    
    // Clear all entities
    clearBoxes();
    
    if (registry.valid(player)) {
        registry.destroy(player);
    }
    if (registry.valid(ground)) {
        registry.destroy(ground);
    }
    if (registry.valid(leftWall)) {
        registry.destroy(leftWall);
    }
    if (registry.valid(rightWall)) {
        registry.destroy(rightWall);
    }
}

void DemoGame::createGround() {
    ground = registry.create();
    
    auto& transform = registry.emplace<TransformComponent>(ground);
    transform.position = {0.0f, 450.0f, 0.0f};
    transform.scale = {800.0f, 20.0f, 1.0f};
    
    auto& sprite = registry.emplace<Sprite>(ground);
    sprite.tint = DARKGRAY;
    
    auto& rigidBody = registry.emplace<RigidBody>(ground);
    rigidBody.type = BodyType::Static;
    rigidBody.mass = 0.0f;
    
    auto& collider = registry.emplace<BoxCollider>(ground);
    collider.size = {800.0f, 20.0f};
    
    spdlog::debug("Created ground at y={}", transform.position.y);
}

void DemoGame::createWalls() {
    // Left wall
    leftWall = registry.create();
    
    auto& leftTransform = registry.emplace<TransformComponent>(leftWall);
    leftTransform.position = {-410.0f, 240.0f, 0.0f};
    leftTransform.scale = {20.0f, 480.0f, 1.0f};
    
    auto& leftSprite = registry.emplace<Sprite>(leftWall);
    leftSprite.tint = DARKGRAY;
    
    auto& leftBody = registry.emplace<RigidBody>(leftWall);
    leftBody.type = BodyType::Static;
    
    auto& leftCollider = registry.emplace<BoxCollider>(leftWall);
    leftCollider.size = {20.0f, 480.0f};
    
    // Right wall
    rightWall = registry.create();
    
    auto& rightTransform = registry.emplace<TransformComponent>(rightWall);
    rightTransform.position = {410.0f, 240.0f, 0.0f};
    rightTransform.scale = {20.0f, 480.0f, 1.0f};
    
    auto& rightSprite = registry.emplace<Sprite>(rightWall);
    rightSprite.tint = DARKGRAY;
    
    auto& rightBody = registry.emplace<RigidBody>(rightWall);
    rightBody.type = BodyType::Static;
    
    auto& rightCollider = registry.emplace<BoxCollider>(rightWall);
    rightCollider.size = {20.0f, 480.0f};
}

void DemoGame::createPlayer() {
    player = registry.create();
    
    auto& transform = registry.emplace<TransformComponent>(player);
    transform.position = {0.0f, 300.0f, 0.0f};
    transform.scale = {40.0f, 40.0f, 1.0f};
    
    auto& sprite = registry.emplace<Sprite>(player);
    sprite.tint = BLUE;
    
    auto& rigidBody = registry.emplace<RigidBody>(player);
    rigidBody.type = BodyType::Dynamic;
    rigidBody.mass = 1.0f;
    rigidBody.restitution = 0.0f;
    rigidBody.friction = 0.3f;
    rigidBody.gravityScale = gravityScale;
    
    auto& collider = registry.emplace<BoxCollider>(player);
    collider.size = {40.0f, 40.0f};
    
    auto& controller = registry.emplace<PlayerController>(player);
    controller.speed = playerSpeed;
    controller.jumpForce = 500.0f;
    
    spdlog::info("Created player entity at ({}, {})", transform.position.x, transform.position.y);
}

void DemoGame::spawnBox(float x, float y) {
    if (boxes.size() >= static_cast<size_t>(maxBoxes)) {
        return;
    }
    
    entt::entity box = registry.create();
    boxes.push_back(box);
    
    auto& transform = registry.emplace<TransformComponent>(box);
    transform.position = {x, y, 0.0f};
    transform.scale = {30.0f, 30.0f, 1.0f};
    
    // Random rotation
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> rotDis(0.0f, 360.0f);
    transform.rotation.z = rotDis(gen);
    
    auto& sprite = registry.emplace<Sprite>(box);
    
    // Random color
    static std::uniform_int_distribution<int> colorDis(0, 4);
    Color colors[] = {RED, GREEN, YELLOW, ORANGE, PURPLE};
    sprite.tint = colors[colorDis(gen)];
    
    auto& rigidBody = registry.emplace<RigidBody>(box);
    rigidBody.type = BodyType::Dynamic;
    rigidBody.mass = 0.5f;
    rigidBody.restitution = 0.3f;
    rigidBody.friction = 0.5f;
    rigidBody.gravityScale = gravityScale;
    
    // Random initial velocity
    static std::uniform_real_distribution<float> velDis(-50.0f, 50.0f);
    rigidBody.velocity = {velDis(gen), 0.0f};
    
    auto& collider = registry.emplace<BoxCollider>(box);
    collider.size = {30.0f, 30.0f};
    
    spdlog::debug("Spawned box at ({}, {}) with color index {}", x, y, colorDis(gen));
}

void DemoGame::handleInput() {
    // Toggle physics debug with F2 (F1 is used for console)
    if (IsKeyPressed(KEY_F2)) {
        togglePhysicsDebug();
        spdlog::info("Physics debug rendering: {}", showPhysicsDebug ? "ON" : "OFF");
    }
    
    // Spawn box at mouse position with left click
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePos = GetMousePosition();
        spawnBox(mousePos.x - 400.0f, mousePos.y - 240.0f);
    }
    
    // Clear boxes with C key
    if (IsKeyPressed(KEY_C)) {
        clearBoxes();
        spdlog::info("Cleared all boxes");
    }
    
    // Reset scene with R key
    if (IsKeyPressed(KEY_R)) {
        resetScene();
        spdlog::info("Reset demo scene");
    }
}

void DemoGame::updateUI() {
    // Draw UI information
    DrawText("Demo Game - Box2D Physics", 10, 10, 20, WHITE);
    DrawText(TextFormat("Boxes: %d/%d", boxes.size(), maxBoxes), 10, 35, 16, WHITE);
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 55, 16, GREEN);
    
    // Instructions
    const char* instructions[] = {
        "Controls:",
        "WASD/Arrows - Move player",
        "Space - Jump",
        "Mouse Click - Spawn box",
        "F2 - Toggle physics debug",
        "C - Clear boxes",
        "R - Reset scene",
        "~ - Open console",
        "ESC - Exit"
    };
    
    int y = 100;
    for (const char* instruction : instructions) {
        DrawText(instruction, 10, y, 14, LIGHTGRAY);
        y += 18;
    }
    
    // Physics debug status
    if (showPhysicsDebug) {
        DrawText("PHYSICS DEBUG ON", GetScreenWidth() - 200, 10, 16, YELLOW);
    }
    
    // Auto spawn status
    if (autoSpawnBoxes) {
        DrawText("AUTO SPAWN ON", GetScreenWidth() - 200, 30, 16, GREEN);
    }
}

void DemoGame::clearBoxes() {
    for (auto entity : boxes) {
        if (registry.valid(entity)) {
            registry.destroy(entity);
        }
    }
    boxes.clear();
}

void DemoGame::resetScene() {
    // Clear existing entities
    clearBoxes();
    
    // Reset player position
    if (registry.valid(player)) {
        auto& transform = registry.get<TransformComponent>(player);
        transform.position = {0.0f, 300.0f, 0.0f};
        
        auto& rigidBody = registry.get<RigidBody>(player);
        rigidBody.velocity = {0.0f, 0.0f};
    }
    
    // Spawn initial boxes again
    for (int i = 0; i < 5; ++i) {
        float x = -200.0f + (i * 100.0f);
        float y = 200.0f + (i * 50.0f);
        spawnBox(x, y);
    }
    
    spawnTimer = 0.0f;
}

void DemoGame::setPlayerSpeed(float speed) {
    playerSpeed = speed;
    if (registry.valid(player)) {
        auto& controller = registry.get<PlayerController>(player);
        controller.speed = speed;
        spdlog::info("Player speed set to: {}", speed);
    }
}

void DemoGame::loadGameSettings() {
    if (Config::isConfigLoaded()) {
        autoSpawnBoxes = Config::getBool("demo.auto_spawn_boxes", true);
        spawnInterval = Config::getFloat("demo.spawn_interval", 2.0f);
        maxBoxes = Config::getInt("demo.max_boxes", 20);
        playerSpeed = Config::getFloat("demo.player_speed", 200.0f);
        gravityScale = Config::getFloat("demo.gravity_scale", 1.0f);
        
        spdlog::info("Loaded demo settings from config: auto_spawn={}, interval={}, max_boxes={}", 
                     autoSpawnBoxes, spawnInterval, maxBoxes);
    }
}