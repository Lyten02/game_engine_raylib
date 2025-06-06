#include <raylib.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <entt/entt.hpp>
#include <fstream>
#include <string>
#include <memory>
#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __linux__
#include <unistd.h>
#endif

// Component definitions
struct TransformComponent {
    Vector3 position{0.0f, 0.0f, 0.0f};
    Vector3 rotation{0.0f, 0.0f, 0.0f};
    Vector3 scale{1.0f, 1.0f, 1.0f};
    
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

struct Sprite {
    Texture2D* texture = nullptr;
    Rectangle sourceRect{0.0f, 0.0f, 0.0f, 0.0f};
    Color tint = WHITE;
    std::string texturePath;
    
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

// Simple resource manager
class ResourceManager {
private:
    std::unordered_map<std::string, Texture2D> textures;
    
public:
    Texture2D* loadTexture(const std::string& path, const std::string& name) {
        if (textures.find(name) != textures.end()) {
            return &textures[name];
        }
        
        Texture2D texture = LoadTexture(path.c_str());
        if (texture.id != 0) {
            textures[name] = texture;
            return &textures[name];
        }
        return nullptr;
    }
    
    void unloadAll() {
        for (auto& [name, texture] : textures) {
            UnloadTexture(texture);
        }
        textures.clear();
    }
};

// Simple game runtime
class GameRuntime {
private:
    entt::registry registry;
    std::unique_ptr<ResourceManager> resourceManager;
    Camera2D camera;
    bool running = false;
    std::filesystem::path executablePath;
    
public:
    bool initialize(const std::string& configPath, const std::filesystem::path& exePath) {
        // Store executable path and change to its directory
        executablePath = exePath.parent_path();
        std::filesystem::current_path(executablePath);
        
        spdlog::info("Changed working directory to: {}", std::filesystem::current_path().string());
        
        // Load config
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            spdlog::error("Failed to open config file: {}", configPath);
            spdlog::info("Looking in: {}", std::filesystem::current_path().string());
            
            // Try with project name prefix for compatibility
            std::string altConfigPath = "{{PROJECT_NAME}}_config.json";
            configFile.open(altConfigPath);
            if (!configFile.is_open()) {
                spdlog::error("Also failed to open: {}", altConfigPath);
                return false;
            }
            spdlog::info("Opened alternative config: {}", altConfigPath);
        }
        
        nlohmann::json config;
        configFile >> config;
        configFile.close();
        
        // Initialize window
        int width = 800;
        int height = 600;
        std::string title = "{{PROJECT_NAME}}";
        
        if (config.contains("window")) {
            width = config["window"].value("width", 800);
            height = config["window"].value("height", 600);
            title = config["window"].value("title", "{{PROJECT_NAME}}");
        }
        
        InitWindow(width, height, title.c_str());
        SetTargetFPS(60);
        
        // Initialize camera
        camera.target = {0.0f, 0.0f};
        camera.offset = {width / 2.0f, height / 2.0f};
        camera.rotation = 0.0f;
        camera.zoom = 1.0f;
        
        // Initialize resource manager
        resourceManager = std::make_unique<ResourceManager>();
        
        running = true;
        return true;
    }
    
    void loadScene(const std::string& scenePath) {
        std::ifstream sceneFile(scenePath);
        if (!sceneFile.is_open()) {
            spdlog::error("Failed to open scene file: {}", scenePath);
            return;
        }
        
        nlohmann::json sceneData;
        sceneFile >> sceneData;
        sceneFile.close();
        
        // Clear existing entities
        registry.clear();
        
        // Load entities
        if (sceneData.contains("entities") && sceneData["entities"].is_array()) {
            for (const auto& entityData : sceneData["entities"]) {
                entt::entity entity = registry.create();
                
                if (entityData.contains("components")) {
                    const auto& components = entityData["components"];
                    
                    // Transform component
                    if (components.contains("Transform")) {
                        auto& transform = registry.emplace<TransformComponent>(entity);
                        transform.from_json(components["Transform"]);
                    }
                    
                    // Sprite component
                    if (components.contains("Sprite")) {
                        auto& sprite = registry.emplace<Sprite>(entity);
                        sprite.from_json(components["Sprite"]);
                        
                        // Load texture
                        if (!sprite.texturePath.empty()) {
                            sprite.texture = resourceManager->loadTexture(
                                "assets/" + sprite.texturePath,
                                sprite.texturePath
                            );
                        }
                    }
                }
            }
        }
    }
    
    void run() {
        while (running && !WindowShouldClose()) {
            // Update
            float deltaTime = GetFrameTime();
            
            // Render
            BeginDrawing();
            ClearBackground(DARKGRAY);
            
            BeginMode2D(camera);
            
            // Render sprites
            auto view = registry.view<TransformComponent, Sprite>();
            for (auto entity : view) {
                const auto& transform = view.get<TransformComponent>(entity);
                const auto& sprite = view.get<Sprite>(entity);
                
                if (sprite.texture) {
                    DrawTextureRec(
                        *sprite.texture,
                        sprite.sourceRect,
                        {transform.position.x, transform.position.y},
                        sprite.tint
                    );
                }
            }
            
            EndMode2D();
            EndDrawing();
        }
    }
    
    void shutdown() {
        registry.clear();
        resourceManager->unloadAll();
        CloseWindow();
    }
};

// Helper function to get executable path
std::filesystem::path getExecutablePath(char* argv0) {
    std::filesystem::path exePath;
    
    #ifdef __APPLE__
        // macOS specific
        char pathbuf[1024];
        uint32_t bufsize = sizeof(pathbuf);
        if (_NSGetExecutablePath(pathbuf, &bufsize) == 0) {
            exePath = std::filesystem::canonical(pathbuf);
        } else {
            exePath = std::filesystem::canonical(argv0);
        }
    #elif _WIN32
        // Windows specific
        char pathbuf[MAX_PATH];
        GetModuleFileNameA(NULL, pathbuf, MAX_PATH);
        exePath = pathbuf;
    #else
        // Linux
        char pathbuf[1024];
        ssize_t len = readlink("/proc/self/exe", pathbuf, sizeof(pathbuf)-1);
        if (len != -1) {
            pathbuf[len] = '\0';
            exePath = pathbuf;
        } else {
            exePath = std::filesystem::canonical(argv0);
        }
    #endif
    
    return exePath;
}

int main(int argc, char* argv[]) {
    // Get executable path
    std::filesystem::path exePath = getExecutablePath(argv[0]);
    
    GameRuntime runtime;
    
    if (!runtime.initialize("game_config.json", exePath)) {
        return -1;
    }
    
    runtime.loadScene("scenes/{{MAIN_SCENE}}.json");
    runtime.run();
    runtime.shutdown();
    
    return 0;
}