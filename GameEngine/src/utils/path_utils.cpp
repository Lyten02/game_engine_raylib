#include "path_utils.h"
#include <spdlog/spdlog.h>

namespace PathUtils {

std::string getProjectRoot() {
    // Start from current directory and look for marker files
    std::filesystem::path current = std::filesystem::current_path();
    
    // Look for GameEngine directory or markers
    while (current != current.root_path()) {
        // Check if we're in the GameEngine directory
        if (current.filename() == "GameEngine") {
            return current.string();
        }
        
        // Check if GameEngine is a subdirectory
        std::filesystem::path gameEnginePath = current / "GameEngine";
        if (std::filesystem::exists(gameEnginePath) && std::filesystem::is_directory(gameEnginePath)) {
            return gameEnginePath.string();
        }
        
        // Check for marker files that indicate we're in the GameEngine directory
        if (std::filesystem::exists(current / "templates") && 
            std::filesystem::exists(current / "src") && 
            std::filesystem::exists(current / "CMakeLists.txt")) {
            return current.string();
        }
        
        current = current.parent_path();
    }
    
    // Fallback: assume we're in build directory and validate
    std::filesystem::path fallback = std::filesystem::current_path().parent_path();
    if (std::filesystem::exists(fallback / "templates") && 
        std::filesystem::exists(fallback / "src")) {
        return fallback.string();
    }
    
    // If even fallback fails, log warning and return current directory
    spdlog::warn("Could not find GameEngine root directory, using current directory");
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