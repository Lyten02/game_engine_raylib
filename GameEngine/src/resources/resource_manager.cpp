#include "resource_manager.h"
#include <iostream>
#include <filesystem>
#include <unordered_set>
#include <chrono>
#include <spdlog/spdlog.h>
#include "../utils/log_limiter.h"

ResourceManager::ResourceManager() {
    // Default texture will be created on first use via std::once_flag
}

ResourceManager::~ResourceManager() {
    unloadAll();
    // Default texture is managed by unique_ptr, will be automatically cleaned up
}

void ResourceManager::createDummyTexture() {
    defaultTexture = std::make_unique<Texture2D>();
    defaultTexture->id = 0;
    defaultTexture->width = 64;
    defaultTexture->height = 64;
    defaultTexture->mipmaps = 1;
    defaultTexture->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    
    // Log only once to avoid spam
    static std::atomic<bool> loggedOnce{false};
    if (!silentMode.load() && !loggedOnce.exchange(true)) {
        if (headlessMode.load()) {
            spdlog::info("[ResourceManager] Created dummy texture for headless mode");
        } else {
            spdlog::info("[ResourceManager] Created dummy texture (RayLib not initialized)");
        }
    }
}

void ResourceManager::createRealTexture() {
    const int size = 64;
    const int checkSize = 8;
    
    // Generate checkerboard image
    Image img = GenImageChecked(size, size, checkSize, checkSize, MAGENTA, BLACK);
    
    // RAII scope guard to ensure image cleanup
    struct ImageGuard {
        Image& img;
        ~ImageGuard() { UnloadImage(img); }
    } imageGuard{img};
    
    // Create texture from image
    defaultTexture = std::make_unique<Texture2D>(LoadTextureFromImage(img));
    
    if (!silentMode.load()) {
        spdlog::info("[ResourceManager] Created default texture (64x64 pink-black checkerboard)");
    }
}

void ResourceManager::createEmergencyFallbackTexture() noexcept {
    try {
        defaultTexture = std::make_unique<Texture2D>();
        defaultTexture->id = 0;
        defaultTexture->width = 64;
        defaultTexture->height = 64;
        defaultTexture->mipmaps = 1;
        defaultTexture->format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        
        if (!silentMode.load()) {
            spdlog::warn("[ResourceManager] Using emergency fallback texture");
        }
    } catch (...) {
        // If we can't even create a dummy texture, the system is critically broken
        spdlog::critical("[ResourceManager] Cannot allocate memory for emergency fallback texture");
        std::terminate();
    }
}

void ResourceManager::createDefaultTextureThreadSafe() {
    try {
        if (headlessMode.load()) {
            // Create dummy texture for headless mode
            createDummyTexture();
            // Log is already in createDummyTexture()
        } else if (!rayLibInitialized.load()) {
            // Create dummy texture when RayLib not initialized
            createDummyTexture();
            // Log is already in createDummyTexture()
        } else {
            // Create real texture with RayLib
            createRealTexture();
        }
    } catch (const std::exception& e) {
        spdlog::error("[ResourceManager] Failed to create default texture: {}", e.what());
        createEmergencyFallbackTexture();
    } catch (...) {
        // Catch any non-std::exception errors (shouldn't happen with RayLib, but be safe)
        spdlog::error("[ResourceManager] Unexpected error creating default texture");
        createEmergencyFallbackTexture();
    }
    
    // Post-condition: defaultTexture is always valid
    if (!defaultTexture) {
        spdlog::critical("[ResourceManager] Default texture is null after initialization - critical system error");
        throw std::runtime_error("ResourceManager: Cannot create default texture - system failure");
    }
}

Texture2D& ResourceManager::getDefaultTexture() {
    // Double-checked locking pattern for thread-safe lazy initialization
    // This pattern is safe on all architectures when using proper memory ordering
    
    // First check with acquire semantics - ensures we see all writes from the initialization
    // This is the fast path that avoids locking after initialization
    if (!defaultTextureInitialized.load(std::memory_order_acquire)) {
        // Take the mutex to ensure only one thread initializes
        std::lock_guard<std::mutex> lock(defaultTextureMutex);
        
        // Double-check if initialization was attempted to prevent repeated exceptions
        if (!defaultTextureInitAttempted.load(std::memory_order_relaxed)) {
            // Mark that we've attempted initialization
            defaultTextureInitAttempted.store(true, std::memory_order_relaxed);
            
            try {
                createDefaultTextureThreadSafe();
                
                // Only mark as initialized if successful
                // Store with release semantics - ensures all initialization writes are visible
                // before other threads see defaultTextureInitialized as true
                defaultTextureInitialized.store(true, std::memory_order_release);
            } catch (const std::exception& e) {
                // Log the error but don't rethrow - we'll handle it below
                spdlog::error("[ResourceManager] Failed to initialize default texture: {}", e.what());
            }
        }
    }
    
    // Memory ordering guarantees:
    // - If we see defaultTextureInitialized == true (via acquire), we're guaranteed to see
    //   the fully initialized defaultTexture (via the release store)
    // - This works correctly on weak memory models like ARM without additional barriers
    
    // Emergency safety check - should never happen with proper initialization
    if (!defaultTexture) {
        spdlog::error("[ResourceManager] Default texture is null after initialization - critical error");
        throw std::runtime_error("[ResourceManager] Default texture initialization failed");
    }
    
    return *defaultTexture;
}

Texture2D* ResourceManager::loadTexture(const std::string& path, const std::string& name) {
    // Simple headless check
    if (headlessMode.load()) {
        // Use LogLimiter for headless mode info
        if (!silentMode.load()) {
            GameEngine::LogLimiter::info(
                "headless_mode_texture",
                "[ResourceManager] Headless mode: using dummy texture for '{}'", 
                name
            );
        }
        return &getDefaultTexture();
    }
    
    // Thread-safe check if already loaded (read lock)
    {
        std::shared_lock<std::shared_mutex> lock(resourceMutex);
        auto it = textures.find(name);
        if (it != textures.end()) {
            if (!silentMode.load()) {
                GameEngine::LogLimiter::info(
                    "texture_already_loaded",
                    "[ResourceManager] Texture '{}' already loaded.", 
                    name
                );
            }
            return &it->second;
        }
    }

    // Check if we can actually load textures
    if (!rayLibInitialized.load()) {
        // Use LogLimiter for uninitialized RayLib info
        if (!silentMode.load()) {
            GameEngine::LogLimiter::info(
                "raylib_not_initialized_texture",
                "[ResourceManager] RayLib not initialized: using default texture for '{}'", 
                name
            );
        }
        return &getDefaultTexture();
    }

    // Load texture WITHOUT holding any locks
    if (!std::filesystem::exists(path)) {
        // Use LogLimiter for missing file warnings
        if (!silentMode.load()) {
            GameEngine::LogLimiter::warn(
                "texture_file_not_found",
                "[ResourceManager] Texture file not found: {} - using default texture for '{}'", 
                path, name
            );
        }
        return &getDefaultTexture();
    }

    Texture2D texture = LoadTexture(path.c_str());
    if (texture.id == 0) {
        // Use LogLimiter for failed load warnings
        if (!silentMode.load()) {
            GameEngine::LogLimiter::warn(
                "texture_load_failed",
                "[ResourceManager] Failed to load texture: {} - using default texture for '{}'", 
                path, name
            );
        }
        return &getDefaultTexture();
    }

    // Store texture with double-check pattern (write lock)
    {
        std::unique_lock<std::shared_mutex> lock(resourceMutex);
        
        // Double-check: another thread might have loaded it while we were loading
        auto it = textures.find(name);
        if (it != textures.end()) {
            // Another thread loaded it first, unload our copy and return existing
            UnloadTexture(texture);
            if (!silentMode.load()) {
                spdlog::info("[ResourceManager] Texture '{}' was loaded by another thread.", name);
            }
            return &it->second;
        }
        
        // We're first to load it
        textures[name] = std::move(texture);
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Loaded texture '{}' from: {}", name, path);
        }
        return &textures[name];
    }
}

Sound* ResourceManager::loadSound(const std::string& path, const std::string& name) {
    // Thread-safe check if already loaded (read lock)
    {
        std::shared_lock<std::shared_mutex> lock(resourceMutex);
        auto it = sounds.find(name);
        if (it != sounds.end()) {
            if (!silentMode.load()) {
                GameEngine::LogLimiter::info(
                    "sound_already_loaded",
                    "[ResourceManager] Sound '{}' already loaded.", 
                    name
                );
            }
            return &it->second;
        }
    }

    // Check if we can actually load sounds
    if (!rayLibInitialized.load() || headlessMode.load()) {
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Cannot load sounds in current mode: '{}'", name);
        }
        return nullptr;
    }

    if (!std::filesystem::exists(path)) {
        if (!silentMode.load()) {
            spdlog::error("[ResourceManager] Sound file not found: {}", path);
        }
        return nullptr;
    }

    Sound sound = LoadSound(path.c_str());
    if (sound.frameCount == 0) {
        if (!silentMode.load()) {
            spdlog::error("[ResourceManager] Failed to load sound: {}", path);
        }
        return nullptr;
    }

    // Thread-safe sound storage (write lock)
    {
        std::unique_lock<std::shared_mutex> lock(resourceMutex);
        sounds[name] = sound;
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Loaded sound '{}' from: {}", name, path);
        }
        return &sounds[name];
    }
}

Texture2D* ResourceManager::getTexture(const std::string& name) {
    // Thread-safe texture lookup (read lock)
    {
        std::shared_lock<std::shared_mutex> lock(resourceMutex);
        auto it = textures.find(name);
        if (it != textures.end()) {
            return &it->second;
        }
    }
    
    // Use LogLimiter for missing texture warnings
    if (!silentMode.load()) {
        GameEngine::LogLimiter::warn(
            "texture_not_found",
            "[ResourceManager] Texture '{}' not found - using default texture", 
            name
        );
    }
    // Return default texture without storing in map
    return &getDefaultTexture();
}

Sound* ResourceManager::getSound(const std::string& name) {
    // Thread-safe sound lookup (read lock)
    std::shared_lock<std::shared_mutex> lock(resourceMutex);
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        return &it->second;
    }
    if (!silentMode.load()) {
        GameEngine::LogLimiter::warn(
            "sound_not_found",
            "[ResourceManager] Sound '{}' not found.", 
            name
        );
    }
    return nullptr;
}

void ResourceManager::unloadAll() {
    if (!silentMode.load()) {
        spdlog::info("[ResourceManager] Unloading all resources...");
    }
    
    // Thread-safe resource unloading (write lock)
    std::unique_lock<std::shared_mutex> lock(resourceMutex);
    
    // Unload textures
    for (auto& [name, texture] : textures) {
        if (!headlessMode.load() && rayLibInitialized.load() && texture.id > 0) {
            UnloadTexture(texture);
        }
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Unloaded texture: {}", name);
        }
    }
    textures.clear();
    
    // Unload sounds
    for (auto& [name, sound] : sounds) {
        if (!headlessMode.load() && rayLibInitialized.load()) {
            UnloadSound(sound);
        }
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Unloaded sound: {}", name);
        }
    }
    sounds.clear();
}

void ResourceManager::unloadTexture(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(resourceMutex);
    auto it = textures.find(name);
    if (it != textures.end()) {
        if (!headlessMode.load() && rayLibInitialized.load() && it->second.id > 0) {
            UnloadTexture(it->second);
        }
        textures.erase(it);
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Unloaded texture: {}", name);
        }
    } else {
        if (!silentMode.load()) {
            GameEngine::LogLimiter::warn(
                "cannot_unload_texture",
                "[ResourceManager] Cannot unload texture '{}' - not found.", 
                name
            );
        }
    }
}

void ResourceManager::unloadSound(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(resourceMutex);
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        if (!headlessMode.load() && rayLibInitialized.load()) {
            UnloadSound(it->second);
        }
        sounds.erase(it);
        if (!silentMode.load()) {
            spdlog::info("[ResourceManager] Unloaded sound: {}", name);
        }
    } else {
        if (!silentMode.load()) {
            GameEngine::LogLimiter::warn(
                "cannot_unload_sound",
                "[ResourceManager] Cannot unload sound '{}' - not found.", 
                name
            );
        }
    }
}

size_t ResourceManager::getUniqueTexturesCount() const noexcept {
    // Thread-safe texture count (read lock)
    std::shared_lock<std::shared_mutex> lock(resourceMutex);
    return textures.size();
}