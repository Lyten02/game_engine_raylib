// Пример использования platformer-toolkit пакета

#include "engine.h"
#include "scene/scene.h"
#include "packages/platformer-toolkit/components/player_controller.h"
#include "packages/platformer-toolkit/components/moving_platform.h"
#include "packages/physics-2d/components/rigidbody.h"
#include "components/transform.h"
#include "components/sprite.h"

void setupPlatformerScene(Scene* scene) {
    // Создаем игрока
    auto player = scene->createEntity("Player");
    
    // Добавляем компоненты
    scene->registry.emplace<TransformComponent>(player, 
        glm::vec3(100.0f, 300.0f, 0.0f),  // position
        glm::vec3(0.0f, 0.0f, 0.0f),      // rotation
        glm::vec3(1.0f, 1.0f, 1.0f)       // scale
    );
    
    scene->registry.emplace<GameEngine::PlayerController>(player);
    
    scene->registry.emplace<GameEngine::RigidBody>(player,
        GameEngine::BodyType::Dynamic,
        1.0f  // mass
    );
    
    scene->registry.emplace<Sprite>(player,
        "assets/player.png",
        WHITE,
        Rectangle{0, 0, 32, 32}  // source rect
    );
    
    // Создаем движущуюся платформу
    auto platform1 = scene->createEntity("MovingPlatform1");
    
    scene->registry.emplace<TransformComponent>(platform1,
        glm::vec3(200.0f, 400.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(3.0f, 0.5f, 1.0f)  // wide platform
    );
    
    auto& movingPlatform = scene->registry.emplace<GameEngine::MovingPlatform>(platform1);
    movingPlatform.type = GameEngine::MovingPlatform::MovementType::Linear;
    movingPlatform.waypoints = {
        glm::vec3(200.0f, 400.0f, 0.0f),
        glm::vec3(400.0f, 400.0f, 0.0f),
        glm::vec3(400.0f, 300.0f, 0.0f),
        glm::vec3(200.0f, 300.0f, 0.0f)
    };
    movingPlatform.speed = 50.0f;
    movingPlatform.loop = true;
    
    scene->registry.emplace<Sprite>(platform1,
        "assets/platform.png",
        BROWN
    );
    
    // Создаем статичные платформы
    for (int i = 0; i < 5; i++) {
        auto ground = scene->createEntity("Ground" + std::to_string(i));
        
        scene->registry.emplace<TransformComponent>(ground,
            glm::vec3(i * 200.0f, 500.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(4.0f, 1.0f, 1.0f)
        );
        
        scene->registry.emplace<GameEngine::RigidBody>(ground,
            GameEngine::BodyType::Static,
            0.0f  // infinite mass
        );
        
        scene->registry.emplace<Sprite>(ground,
            "assets/ground.png",
            DARKGRAY
        );
    }
    
    // Создаем checkpoint
    auto checkpoint = scene->createEntity("Checkpoint1");
    
    scene->registry.emplace<TransformComponent>(checkpoint,
        glm::vec3(600.0f, 450.0f, 0.0f)
    );
    
    scene->registry.emplace<GameEngine::Checkpoint>(checkpoint,
        false,  // not activated
        glm::vec3(600.0f, 450.0f, 0.0f)  // respawn position
    );
    
    scene->registry.emplace<Sprite>(checkpoint,
        "assets/checkpoint.png",
        GOLD
    );
}

// В вашей игровой логике
void updateGame(Scene* scene, float deltaTime) {
    // Системы из пакетов обновляются автоматически!
    // PlayerMovementSystem и PlatformSystem уже работают
    
    // Дополнительная логика игры
    auto checkpoints = scene->registry.view<GameEngine::Checkpoint, TransformComponent>();
    auto players = scene->registry.view<GameEngine::PlayerController, TransformComponent>();
    
    // Проверка активации чекпоинтов
    for (auto checkpoint : checkpoints) {
        auto& cp = checkpoints.get<GameEngine::Checkpoint>(checkpoint);
        auto& cpTransform = checkpoints.get<TransformComponent>(checkpoint);
        
        for (auto player : players) {
            auto& playerTransform = players.get<TransformComponent>(player);
            
            // Простая проверка расстояния
            float distance = glm::distance(playerTransform.position, cpTransform.position);
            if (distance < 50.0f && !cp.isActivated) {
                cp.isActivated = true;
                cp.animationTime = 0.0f;
                cp.showParticles = true;
                
                // Сохраняем позицию респавна
                // В реальной игре это бы сохранялось в GameState
                std::cout << "Checkpoint activated!" << std::endl;
            }
        }
    }
}