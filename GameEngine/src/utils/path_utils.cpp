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
            return current.string() + "/";
        }
        
        // Check if GameEngine is a subdirectory
        std::filesystem::path gameEnginePath = current / "GameEngine";
        if (std::filesystem::exists(gameEnginePath) && std::filesystem::is_directory(gameEnginePath)) {
            return gameEnginePath.string() + "/";
        }
        
        // Check for marker files that indicate we're in the GameEngine directory
        if (std::filesystem::exists(current / "templates") && 
            std::filesystem::exists(current / "src") && 
            std::filesystem::exists(current / "CMakeLists.txt")) {
            return current.string() + "/";
        }
        
        current = current.parent_path();
    }
    
    // Fallback: assume we're in build directory
    return "../";
}

std::string getTemplatePath() {
    std::string projectRoot = getProjectRoot();
    return projectRoot + "templates/";
}

std::string resolveTemplatePath(const std::string& relativePath) {
    // Remove leading slashes if any
    std::string cleanPath = relativePath;
    if (!cleanPath.empty() && cleanPath[0] == '/') {
        cleanPath = cleanPath.substr(1);
    }
    
    std::string fullPath = getTemplatePath() + cleanPath;
    
    // Verify the path exists
    if (!std::filesystem::exists(fullPath)) {
        spdlog::warn("Template path does not exist: {}", fullPath);
        
        // Try alternative paths
        std::vector<std::string> alternatives = {
            "../templates/" + cleanPath,
            "templates/" + cleanPath,
            "../../templates/" + cleanPath,
            "../GameEngine/templates/" + cleanPath
        };
        
        for (const auto& alt : alternatives) {
            if (std::filesystem::exists(alt)) {
                spdlog::info("Using alternative template path: {}", alt);
                return alt;
            }
        }
    }
    
    return fullPath;
}

} // namespace PathUtils