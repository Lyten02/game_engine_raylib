#pragma once

#include <filesystem>
#include <string>

namespace GameEngine {

class EnginePaths {
private:
    static std::filesystem::path engineRoot;
    static bool initialized;
    
    static void ensureInitialized();
    
public:
    // Initialize the engine paths system
    static void initialize();
    
    // Get engine root directory
    static std::filesystem::path getEngineRoot();
    
    // Get specific directories
    static std::filesystem::path getProjectsDir();
    static std::filesystem::path getOutputDir();
    static std::filesystem::path getBuildDir();
    static std::filesystem::path getDependenciesDir();
    static std::filesystem::path getTemplatesDir();
    static std::filesystem::path getLogsDir();
    static std::filesystem::path getConfigFile();
    
    // Get project-specific paths
    static std::filesystem::path getProjectDir(const std::string& projectName);
    static std::filesystem::path getProjectOutputDir(const std::string& projectName);
    static std::filesystem::path getProjectBuildDir(const std::string& projectName);
    
    // Convert to absolute path relative to engine root
    static std::filesystem::path makeAbsolute(const std::filesystem::path& relativePath);
    
    // Get relative path from engine root
    static std::filesystem::path makeRelative(const std::filesystem::path& absolutePath);
    
    // Display all paths (for debugging)
    static void displayPaths();
};

} // namespace GameEngine