#include "game_logic_manager.h"
#include <algorithm>

#ifdef GAME_LOGIC_TEST
    // Simple logging stubs for testing
    namespace spdlog {
        inline void info(const std::string& msg) {}
        inline void error(const std::string& msg) {}
        inline void warn(const std::string& msg) {}
        template<typename... Args>
        inline void info(const std::string& fmt, Args... args) {}
        template<typename... Args>
        inline void error(const std::string& fmt, Args... args) {}
    }
#else
#include <spdlog/spdlog.h>
#endif

GameLogicManager::~GameLogicManager() {
    if (initialized) {
        shutdown();
    }
}

bool GameLogicManager::initialize() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (initialized) {
            spdlog::warn("GameLogicManager already initialized");
            return true;
        }
        
        initialized = true;
    }
    
    spdlog::info("Initializing GameLogicManager");
    
    // Initialize plugin manager
    pluginManager = std::make_unique<GameEngine::GameLogicPluginManager>();
    
    // Register built-in game logic factories (without holding the lock)
    registerBuiltinLogics();
    
    return true;
}

void GameLogicManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!initialized) {
        return;
    }
    
    spdlog::info("Shutting down GameLogicManager");
    
    // Shutdown all active logics
    for (auto& logic : activeLogics) {
        if (logic) {
            logic->shutdown();
        }
    }
    
    activeLogics.clear();
    registeredFactories.clear();
    
    // Cleanup plugin manager
    if (pluginManager) {
        pluginManager.reset();
    }
    
    initialized = false;
}

void GameLogicManager::registerLogicFactory(const std::string& name, GameLogicFactory factory) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!initialized) {
        spdlog::error("GameLogicManager not initialized");
        return;
    }
    
    if (factory == nullptr) {
        spdlog::error("Cannot register null factory for: {}", name);
        return;
    }
    
    registeredFactories[name] = factory;
    spdlog::info("Registered game logic factory: {}", name);
}

bool GameLogicManager::createLogic(const std::string& name, entt::registry& registry) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!initialized) {
        spdlog::error("GameLogicManager not initialized");
        return false;
    }
    
    // First try local factories
    auto it = registeredFactories.find(name);
    if (it != registeredFactories.end()) {
        try {
            auto logic = it->second();
            if (logic) {
                logic->initialize(registry);
                activeLogics.push_back(std::move(logic));
                spdlog::info("Created game logic instance: {}", name);
                return true;
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to create game logic {}: {}", name, e.what());
        }
    }
    
    // Try plugin manager
    if (pluginManager) {
        try {
            auto logic = pluginManager->createGameLogic(name);
            if (logic) {
                logic->initialize(registry);
                activeLogics.push_back(std::move(logic));
                spdlog::info("Created game logic instance from plugin: {}", name);
                return true;
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to create game logic {} from plugin: {}", name, e.what());
        }
    }
    
    spdlog::error("Game logic factory not found: {}", name);
    return false;
}

void GameLogicManager::update(entt::registry& registry, float deltaTime, const InputState& input) {
    if (!initialized) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& logic : activeLogics) {
        if (logic) {
            try {
                logic->update(registry, deltaTime, input);
            } catch (const std::exception& e) {
                spdlog::error("Error in game logic update: {}", e.what());
            }
        }
    }
}

void GameLogicManager::fixedUpdate(entt::registry& registry, float fixedDeltaTime, const InputState& input) {
    if (!initialized) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& logic : activeLogics) {
        if (logic) {
            try {
                logic->fixedUpdate(registry, fixedDeltaTime, input);
            } catch (const std::exception& e) {
                spdlog::error("Error in game logic fixed update: {}", e.what());
            }
        }
    }
}

void GameLogicManager::lateUpdate(entt::registry& registry, float deltaTime, const InputState& input) {
    if (!initialized) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& logic : activeLogics) {
        if (logic) {
            try {
                logic->lateUpdate(registry, deltaTime, input);
            } catch (const std::exception& e) {
                spdlog::error("Error in game logic late update: {}", e.what());
            }
        }
    }
}

void GameLogicManager::onEntityCreated(entt::registry& registry, entt::entity entity) {
    if (!initialized) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& logic : activeLogics) {
        if (logic) {
            try {
                logic->onEntityCreated(registry, entity);
            } catch (const std::exception& e) {
                spdlog::error("Error in game logic onEntityCreated: {}", e.what());
            }
        }
    }
}

void GameLogicManager::onEntityDestroyed(entt::registry& registry, entt::entity entity) {
    if (!initialized) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& logic : activeLogics) {
        if (logic) {
            try {
                logic->onEntityDestroyed(registry, entity);
            } catch (const std::exception& e) {
                spdlog::error("Error in game logic onEntityDestroyed: {}", e.what());
            }
        }
    }
}

std::vector<std::string> GameLogicManager::getActiveLogics() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<std::string> names;
    for (const auto& logic : activeLogics) {
        if (logic) {
            names.push_back(logic->getName());
        }
    }
    return names;
}

bool GameLogicManager::removeLogic(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!initialized) {
        spdlog::error("GameLogicManager not initialized");
        return false;
    }
    
    auto it = std::remove_if(activeLogics.begin(), activeLogics.end(),
        [&name](const std::unique_ptr<IGameLogic>& logic) {
            if (logic && logic->getName() == name) {
                logic->shutdown();
                return true;
            }
            return false;
        });
    
    if (it != activeLogics.end()) {
        activeLogics.erase(it, activeLogics.end());
        spdlog::info("Removed game logic: {}", name);
        return true;
    }
    
    return false;
}

void GameLogicManager::clearLogics() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!initialized) {
        return;
    }
    
    for (auto& logic : activeLogics) {
        if (logic) {
            logic->shutdown();
        }
    }
    
    activeLogics.clear();
    spdlog::info("Cleared all game logics");
}

void GameLogicManager::registerBuiltinLogics() {
    // No built-in game logics in the engine
    // Game logic should come from project plugins
    spdlog::info("GameLogicManager: No built-in game logics registered");
}

bool GameLogicManager::loadProjectPlugins(const std::string& projectPath) {
    if (!initialized || !pluginManager) {
        spdlog::error("GameLogicManager not initialized or plugin manager missing");
        return false;
    }
    
    spdlog::info("Loading plugins for project: {}", projectPath);
    return pluginManager->loadProjectPlugins(projectPath);
}

void GameLogicManager::unloadAllPlugins() {
    if (pluginManager) {
        pluginManager->clearAll();
        spdlog::info("Unloaded all plugins");
    }
}