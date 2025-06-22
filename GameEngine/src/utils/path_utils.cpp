#include "path_utils.h"
#include <spdlog/spdlog.h>
#include <cstdlib>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __linux__
#include <unistd.h>
#endif

namespace PathUtils {

// Helper function to get executable path
std::filesystem::path getExecutablePath() {
    std::filesystem::path exePath;
    
#ifdef __APPLE__
        // macOS specific
        char pathbuf[1024];
        uint32_t bufsize = sizeof(pathbuf);
        if (_NSGetExecutablePath(pathbuf, &bufsize) == 0) {
            exePath = std::filesystem::canonical(pathbuf);
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
        }
#endif
    
    return exePath;
}

std::string getProjectRoot() {
    // Strategy 1: Start from executable location
    // This is most reliable when running from build directory
    std::filesystem::path exePath = getExecutablePath();
    if (!exePath.empty()) {
        std::filesystem::path searchPath = exePath.parent_path();
        
        // Search up from executable location
        while (searchPath != searchPath.root_path()) {
            // Check if we're in the GameEngine directory
            if (searchPath.filename() == "GameEngine") {
                spdlog::debug("Found GameEngine root from executable path: {}", searchPath.string());
                return searchPath.string();
            }
            
            // Check if GameEngine is a subdirectory
            std::filesystem::path gameEnginePath = searchPath / "GameEngine";
            if (std::filesystem::exists(gameEnginePath) && std::filesystem::is_directory(gameEnginePath)) {
                spdlog::debug("Found GameEngine subdirectory from executable path: {}", gameEnginePath.string());
                return gameEnginePath.string();
            }
            
            // Check for marker files that indicate we're in the GameEngine directory
            if (std::filesystem::exists(searchPath / "templates") && 
                std::filesystem::exists(searchPath / "src") && 
                std::filesystem::exists(searchPath / "CMakeLists.txt")) {
                spdlog::debug("Found GameEngine root by markers from executable path: {}", searchPath.string());
                return searchPath.string();
            }
            
            searchPath = searchPath.parent_path();
        }
    }
    
    // Strategy 2: Check environment variable (useful for tests)
    const char* engineRoot = std::getenv("GAMEENGINE_ROOT");
    if (engineRoot && std::filesystem::exists(engineRoot)) {
        spdlog::debug("Using GAMEENGINE_ROOT environment variable: {}", engineRoot);
        return engineRoot;
    }
    
    // Strategy 3: Start from current directory (fallback)
    std::filesystem::path current = std::filesystem::current_path();
    
    // Look for GameEngine directory or markers
    while (current != current.root_path()) {
        // Check if we're in the GameEngine directory
        if (current.filename() == "GameEngine") {
            spdlog::debug("Found GameEngine root from current directory: {}", current.string());
            return current.string();
        }
        
        // Check if GameEngine is a subdirectory
        std::filesystem::path gameEnginePath = current / "GameEngine";
        if (std::filesystem::exists(gameEnginePath) && std::filesystem::is_directory(gameEnginePath)) {
            spdlog::debug("Found GameEngine subdirectory from current directory: {}", gameEnginePath.string());
            return gameEnginePath.string();
        }
        
        // Check for marker files that indicate we're in the GameEngine directory
        if (std::filesystem::exists(current / "templates") && 
            std::filesystem::exists(current / "src") && 
            std::filesystem::exists(current / "CMakeLists.txt")) {
            spdlog::debug("Found GameEngine root by markers from current directory: {}", current.string());
            return current.string();
        }
        
        current = current.parent_path();
    }
    
    // Strategy 4: Common development paths
    std::vector<std::filesystem::path> commonPaths = {
        "/Users/konstantin/Desktop/Code/GameEngineRayLib/GameEngine",
        std::filesystem::current_path().parent_path() / "GameEngine",
        std::filesystem::current_path().parent_path().parent_path() / "GameEngine"
    };
    
    // Add home directory path if available
    const char* homeDir = std::getenv("HOME");
    if (homeDir) {
        commonPaths.push_back(std::filesystem::path(homeDir) / "Desktop/Code/GameEngineRayLib/GameEngine");
    }
    
    for (const auto& path : commonPaths) {
        if (std::filesystem::exists(path / "templates") && 
            std::filesystem::exists(path / "src")) {
            spdlog::debug("Found GameEngine root at common path: {}", path.string());
            return path.string();
        }
    }
    
    // If all strategies fail, log error with helpful information
    spdlog::error("Could not find GameEngine root directory!");
    spdlog::error("  Executable path: {}", exePath.string());
    spdlog::error("  Current directory: {}", std::filesystem::current_path().string());
    spdlog::error("  Tried environment variable GAMEENGINE_ROOT: {}", 
                  engineRoot ? engineRoot : "not set");
    
    // Return a reasonable fallback
    return std::filesystem::current_path().string();
}

std::string getTemplatePath() {
    std::filesystem::path projectRoot = getProjectRoot();
    std::filesystem::path templatePath = projectRoot / "templates";
    
    // Validate template directory exists
    if (!std::filesystem::exists(templatePath)) {
        spdlog::warn("Template directory not found at: {}", templatePath.string());
    }
    
    return templatePath.string();
}

std::string resolveTemplatePath(const std::string& relativePath) {
    // Remove leading slashes if any
    std::string cleanPath = relativePath;
    if (!cleanPath.empty() && cleanPath[0] == '/') {
        cleanPath = cleanPath.substr(1);
    }
    
    // Use filesystem::path for proper path concatenation
    std::filesystem::path templateBasePath = getTemplatePath();
    std::filesystem::path fullPath = templateBasePath / cleanPath;
    
    // Verify the path exists
    if (!std::filesystem::exists(fullPath)) {
        spdlog::warn("Template path does not exist: {}", fullPath.string());
        
        // Try alternative paths using filesystem::path
        std::filesystem::path projectRoot = getProjectRoot();
        std::vector<std::filesystem::path> alternatives = {
            projectRoot.parent_path() / "templates" / cleanPath,
            std::filesystem::current_path() / "templates" / cleanPath,
            projectRoot.parent_path().parent_path() / "templates" / cleanPath,
            projectRoot.parent_path() / "GameEngine" / "templates" / cleanPath
        };
        
        for (const auto& alt : alternatives) {
            if (std::filesystem::exists(alt)) {
                spdlog::info("Using alternative template path: {}", alt.string());
                return alt.string();
            }
        }
    }
    
    return fullPath.string();
}

} // namespace PathUtils