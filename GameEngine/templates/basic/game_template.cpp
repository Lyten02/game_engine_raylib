#include <raylib.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <entt/entt.hpp>
#include <fstream>
#include <string>
#include <memory>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <dlfcn.h>  // For dynamic library loading

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __linux__
#include <unistd.h>
#endif

// Input state structure to pass keyboard/mouse state to game logic
struct InputState {
    std::unordered_map<int, bool> keys;
    std::unordered_map<int, bool> keysPressed;
    std::unordered_map<int, bool> keysReleased;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    std::unordered_map<int, bool> mouseButtons;
    
    bool isKeyDown(int key) const {
        auto it = keys.find(key);
        return it != keys.end() && it->second;
    }
    
    bool isKeyPressed(int key) const {
        auto it = keysPressed.find(key);
        return it != keysPressed.end() && it->second;
    }
    
    bool isKeyReleased(int key) const {
        auto it = keysReleased.find(key);
        return it != keysReleased.end() && it->second;
    }
};

// Game logic interface
class IGameLogic {
public:
    virtual ~IGameLogic() = default;
    virtual void initialize(entt::registry& registry) = 0;
    virtual void update(entt::registry& registry, float deltaTime, const InputState& input) = 0;
    virtual void shutdown() = 0;
    virtual std::string getName() const = 0;
};

// Camera component
struct CameraComponent {
    Vector2 target = {0.0f, 0.0f};
    Vector2 offset = {640.0f, 360.0f};
    float rotation = 0.0f;
    float zoom = 1.0f;
    bool active = true;
};

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
    Texture2D defaultTexture;
    
    void createDefaultTexture() {
        const int size = 64;
        const int checkSize = 8;
        
        // Create image for pink-black checkerboard
        Image img = GenImageChecked(size, size, checkSize, checkSize, MAGENTA, BLACK);
        
        // Create texture from image
        defaultTexture = LoadTextureFromImage(img);
        UnloadImage(img);
        
        spdlog::info("[ResourceManager] Created default texture (64x64 pink-black checkerboard)");
    }
    
public:
    ResourceManager() {
        createDefaultTexture();
    }
    
    ~ResourceManager() {
        UnloadTexture(defaultTexture);
    }
    
    Texture2D* loadTexture(const std::string& path, const std::string& name) {
        if (textures.find(name) != textures.end()) {
            return &textures[name];
        }
        
        if (!std::filesystem::exists(path)) {
            spdlog::warn("[ResourceManager] Texture file not found: {}", path);
            spdlog::warn("[ResourceManager] Using default texture for '{}'", name);
            textures[name] = defaultTexture;
            return &textures[name];
        }
        
        Texture2D texture = LoadTexture(path.c_str());
        if (texture.id != 0) {
            textures[name] = texture;
            spdlog::info("[ResourceManager] Loaded texture '{}' from: {}", name, path);
            return &textures[name];
        }
        
        spdlog::warn("[ResourceManager] Failed to load texture: {}", path);
        spdlog::warn("[ResourceManager] Using default texture for '{}'", name);
        textures[name] = defaultTexture;
        return &textures[name];
    }
    
    void unloadAll() {
        for (auto& [name, texture] : textures) {
            // Don't unload if it's the default texture
            if (texture.id != defaultTexture.id) {
                UnloadTexture(texture);
            }
        }
        textures.clear();
    }
};

// Plugin manager for loading game logic
class PluginManager {
private:
    std::unordered_map<std::string, void*> loadedLibraries;
    std::unordered_map<std::string, std::function<std::unique_ptr<IGameLogic>()>> gameLogicFactories;
    std::unordered_set<std::string> allowedPaths;
    bool securityEnabled = true;

public:
    PluginManager() {
        // Add allowed plugin paths for security
        allowedPaths.insert("packages");
        allowedPaths.insert("./packages");
    }

    ~PluginManager() {
        // Unload all libraries
        for (auto& [name, handle] : loadedLibraries) {
            if (handle) {
                dlclose(handle);
            }
        }
    }
    
    bool loadPlugin(const std::string& path, const std::string& name) {
        // Security check: validate plugin path
        if (securityEnabled && !isPathAllowed(path)) {
            spdlog::error("Plugin path not allowed: {}", path);
            return false;
        }
        
        // Check if already loaded
        if (loadedLibraries.find(name) != loadedLibraries.end()) {
            spdlog::warn("Plugin already loaded: {}", name);
            return true;
        }
        
        // Check if file exists
        if (!std::filesystem::exists(path)) {
            spdlog::error("Plugin file not found: {}", path);
            return false;
        }
        
        void* handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!handle) {
            spdlog::error("Failed to load plugin {}: {}", path, dlerror());
            return false;
        }
        
        // Verify plugin has required exports
        void* initFunc = dlsym(handle, "initializePlugin");
        if (!initFunc) {
            spdlog::error("Plugin {} missing required export: initializePlugin", name);
            dlclose(handle);
            return false;
        }
        
        loadedLibraries[name] = handle;
        spdlog::info("Loaded plugin: {}", name);
        return true;
    }
    
private:
    bool isPathAllowed(const std::string& path) {
        std::filesystem::path pluginPath(path);
        std::filesystem::path canonicalPath = std::filesystem::canonical(pluginPath.parent_path());
        
        for (const auto& allowedPath : allowedPaths) {
            std::filesystem::path allowedCanonical = std::filesystem::canonical(allowedPath);
            
            // Check if plugin path is within allowed directory
            auto relative = std::filesystem::relative(canonicalPath, allowedCanonical);
            if (!relative.empty() && relative.native()[0] != '.') {
                return true;
            }
        }
        
        return false;
    }
    
public:
    
    void registerGameLogicFactory(const std::string& name, std::function<std::unique_ptr<IGameLogic>()> factory) {
        gameLogicFactories[name] = factory;
        spdlog::info("Registered game logic factory: {}", name);
    }
    
    std::unique_ptr<IGameLogic> createGameLogic(const std::string& name) {
        auto it = gameLogicFactories.find(name);
        if (it != gameLogicFactories.end()) {
            return it->second();
        }
        return nullptr;
    }
    
    bool unloadPlugin(const std::string& name) {
        auto it = loadedLibraries.find(name);
        if (it != loadedLibraries.end()) {
            dlclose(it->second);
            loadedLibraries.erase(it);
            spdlog::info("Unloaded plugin: {}", name);
            
            // Remove associated game logic factories
            auto factoryIt = gameLogicFactories.find(name);
            if (factoryIt != gameLogicFactories.end()) {
                gameLogicFactories.erase(factoryIt);
            }
            
            return true;
        }
        return false;
    }
    
    void disableSecurity() {
        securityEnabled = false;
        spdlog::warn("Plugin security disabled - use only for development!");
    }
    
    std::vector<std::string> getLoadedPlugins() const {
        std::vector<std::string> plugins;
        for (const auto& [name, handle] : loadedLibraries) {
            plugins.push_back(name);
        }
        return plugins;
    }
};

// Simple game runtime
class GameRuntime {
private:
    entt::registry registry;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<PluginManager> pluginManager;
    std::unique_ptr<IGameLogic> gameLogic;
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
        
        // Initialize plugin manager
        pluginManager = std::make_unique<PluginManager>();
        
        // Load project dependencies and game logic
        if (config.contains("dependencies") && config["dependencies"].is_array()) {
            for (const auto& dep : config["dependencies"]) {
                std::string depName = dep.get<std::string>();
                loadDependency(depName);
            }
        }
        
        // Initialize game logic
        if (config.contains("game_logic")) {
            std::string gameLogicName = config["game_logic"];
            gameLogic = pluginManager->createGameLogic(gameLogicName);
            if (gameLogic) {
                spdlog::info("Created game logic: {}", gameLogicName);
                gameLogic->initialize(registry);
            } else {
                spdlog::warn("Failed to create game logic: {}", gameLogicName);
            }
        }
        
        running = true;
        return true;
    }
    
    bool loadDependency(const std::string& depName) {
        // Try to load package
        std::filesystem::path packageDir = std::filesystem::path("packages") / depName;
        std::filesystem::path packageJson = packageDir / "package.json";
        
        if (!std::filesystem::exists(packageJson)) {
            spdlog::warn("Package not found: {}", depName);
            return false;
        }
        
        // Read package.json
        std::ifstream packageFile(packageJson);
        nlohmann::json packageData;
        packageFile >> packageData;
        packageFile.close();
        
        // Load plugin if specified
        if (packageData.contains("plugin")) {
            const auto& plugin = packageData["plugin"];
            std::string library = plugin.value("library", "");
            
            if (!library.empty()) {
                std::filesystem::path libraryPath = packageDir / library;
                if (std::filesystem::exists(libraryPath)) {
                    return pluginManager->loadPlugin(libraryPath.string(), depName);
                } else {
                    spdlog::warn("Plugin library not found: {}", libraryPath.string());
                }
            }
        }
        
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
            
            // Update game logic
            if (gameLogic) {
                // Create input state
                InputState inputState;
                
                // Common game keys
                std::vector<int> keysToCheck = {
                    KEY_A, KEY_S, KEY_D, KEY_W,
                    KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,
                    KEY_SPACE, KEY_ENTER, KEY_ESCAPE,
                    KEY_LEFT_SHIFT, KEY_LEFT_CONTROL, KEY_LEFT_ALT
                };
                
                for (int key : keysToCheck) {
                    inputState.keys[key] = IsKeyDown(key);
                    inputState.keysPressed[key] = IsKeyPressed(key);
                    inputState.keysReleased[key] = IsKeyReleased(key);
                }
                
                // Mouse position
                inputState.mouseX = GetMouseX();
                inputState.mouseY = GetMouseY();
                
                // Mouse buttons
                inputState.mouseButtons[MOUSE_LEFT_BUTTON] = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
                inputState.mouseButtons[MOUSE_RIGHT_BUTTON] = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);
                inputState.mouseButtons[MOUSE_MIDDLE_BUTTON] = IsMouseButtonDown(MOUSE_MIDDLE_BUTTON);
                
                gameLogic->update(registry, deltaTime, inputState);
            }
            
            // Update camera from CameraComponent if present
            auto cameraView = registry.view<CameraComponent>();
            for (auto entity : cameraView) {
                const auto& cameraComp = cameraView.get<CameraComponent>(entity);
                if (cameraComp.active) {
                    camera.target = cameraComp.target;
                    camera.offset = cameraComp.offset;
                    camera.rotation = cameraComp.rotation;
                    camera.zoom = cameraComp.zoom;
                    break; // Use first active camera
                }
            }
            
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
                    // Render textured sprite
                    DrawTextureRec(
                        *sprite.texture,
                        sprite.sourceRect,
                        {transform.position.x, transform.position.y},
                        sprite.tint
                    );
                } else {
                    // Render colored rectangle when no texture
                    Rectangle rect;
                    
                    // Use transform scale if source rect has no size
                    if (sprite.sourceRect.width <= 0 || sprite.sourceRect.height <= 0) {
                        rect = {
                            transform.position.x - transform.scale.x / 2,
                            transform.position.y - transform.scale.y / 2,
                            transform.scale.x,
                            transform.scale.y
                        };
                    } else {
                        // Use source rect dimensions
                        rect = {
                            transform.position.x - sprite.sourceRect.width / 2,
                            transform.position.y - sprite.sourceRect.height / 2,
                            sprite.sourceRect.width,
                            sprite.sourceRect.height
                        };
                    }
                    
                    DrawRectangleRec(rect, sprite.tint);
                }
            }
            
            EndMode2D();
            EndDrawing();
        }
    }
    
    void shutdown() {
        // Shutdown game logic
        if (gameLogic) {
            gameLogic->shutdown();
            gameLogic.reset();
        }
        
        registry.clear();
        resourceManager->unloadAll();
        pluginManager.reset();
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