#pragma once

#include "scene/scene.h"
#include <vector>
#include <entt/entt.hpp>

class DemoGame : public Scene {
private:
    entt::entity player;
    std::vector<entt::entity> boxes;
    entt::entity ground;
    entt::entity leftWall;
    entt::entity rightWall;
    
    bool showPhysicsDebug = false;
    float spawnTimer = 0.0f;
    float spawnInterval = 2.0f;
    int maxBoxes = 20;
    bool autoSpawnBoxes = true;
    
    // Game settings from config
    float playerSpeed = 200.0f;
    float gravityScale = 1.0f;
    
    void createGround();
    void createWalls();
    void createPlayer();
    void loadGameSettings();
    
public:
    DemoGame() = default;
    ~DemoGame() override = default;
    
    void onCreate() override;
    void onUpdate(float deltaTime) override;
    void onDestroy() override;
    
    void spawnBox(float x, float y);
    void handleInput();
    void updateUI();
    void clearBoxes();
    void resetScene();
    void togglePhysicsDebug() { showPhysicsDebug = !showPhysicsDebug; }
    void setPlayerSpeed(float speed);
    void setAutoSpawn(bool enable) { autoSpawnBoxes = enable; }
    
    bool isPhysicsDebugEnabled() const { return showPhysicsDebug; }
};