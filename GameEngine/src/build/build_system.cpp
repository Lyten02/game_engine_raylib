#include "build_system.h"
#include "../project/project.h"
#include "../utils/file_utils.h"
#include "../utils/string_utils.h"
#include "../utils/path_utils.h"
#include "../utils/engine_paths.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <spdlog/spdlog.h>

namespace GameEngine {

bool BuildSystem::buildProject(Project* project, const std::string& buildConfig) {
    if (!project) {
        spdlog::error("BuildSystem: No project to build");
        return false;
    }
    
    spdlog::info("Building project: {} ({})", project->getName(), buildConfig);
    
    std::string projectName = project->getName();
    std::string buildDir = EnginePaths::getProjectOutputDir(projectName).string();
    
    // Create build directory structure
    if (!createBuildDirectory(projectName)) {
        return false;
    }
    
    // Generate game code from template
    if (!generateGameCode(project, buildDir)) {
        spdlog::error("Failed to generate game code");
        return false;
    }
    
    // Generate CMakeLists.txt
    if (!generateCMakeLists(project, buildDir)) {
        spdlog::error("Failed to generate CMakeLists.txt");
        return false;
    }
    
    // Copy runtime library
    if (!copyRuntimeLibrary(buildDir)) {
        spdlog::error("Failed to copy runtime library");
        return false;
    }
    
    // Process and copy scenes
    if (!processScenes(project, buildDir)) {
        spdlog::error("Failed to process scenes");
        return false;
    }
    
    // Package assets
    if (!packageAssets(project, buildDir)) {
        spdlog::error("Failed to package assets");
        return false;
    }
    
    // Compile the project
    if (!compileProject(project, buildDir, buildDir)) {
        spdlog::error("Failed to compile project");
        return false;
    }
    
    spdlog::info("Project built successfully: {}/game", buildDir);
    return true;
}

bool BuildSystem::generateGameCode(Project* project, const std::string& outputDir) {
    try {
        // Read game template
        std::filesystem::path templatePath = EnginePaths::getTemplatesDir() / "basic" / "game_template.cpp";
        if (!std::filesystem::exists(templatePath)) {
            spdlog::error("Game template not found: {}", templatePath.string());
            spdlog::error("Templates directory: {}", EnginePaths::getTemplatesDir().string());
            spdlog::error("Current directory: {}", std::filesystem::current_path().string());
            return false;
        }
        
        std::string templateContent = FileUtils::readFile(templatePath.string());
        
        // Process template variables
        std::string gameCode = processTemplate(templateContent, project);
        
        // Write main.cpp
        std::ofstream file(outputDir + "/main.cpp");
        if (!file.is_open()) {
            spdlog::error("Failed to create main.cpp");
            return false;
        }
        
        file << gameCode;
        file.close();
        
        spdlog::info("Generated main.cpp for project");
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to generate game code: {}", e.what());
        return false;
    }
}

bool BuildSystem::compileProject(Project* project, const std::string& projectDir, const std::string& outputPath) {
    try {
        // Create build directory for CMake
        std::string buildDir = projectDir + "/build";
        std::filesystem::create_directories(buildDir);
        
        // Change to build directory and run CMake
        std::string currentDir = std::filesystem::current_path().string();
        std::filesystem::current_path(buildDir);
        
        // Configure with CMake - explicitly set compilers on macOS
        std::string cmakeCmd = "cmake -DCMAKE_BUILD_TYPE=Release";
        #ifdef __APPLE__
            // On macOS, explicitly set the compilers to avoid issues
            cmakeCmd += " -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++";
        #endif
        cmakeCmd += " ..";
        
        int result = std::system(cmakeCmd.c_str());
        if (result != 0) {
            // Check if CMakeLists.txt was generated despite warnings
            if (!std::filesystem::exists("Makefile") && !std::filesystem::exists("build.ninja") && 
                !std::filesystem::exists(project->getName() + ".sln") && !std::filesystem::exists("CMakeCache.txt")) {
                spdlog::error("CMake configuration failed - no build files generated");
                std::filesystem::current_path(currentDir);
                return false;
            }
            spdlog::warn("CMake returned non-zero exit code but build files exist - likely due to warnings");
        }
        
        // Build with CMake
        std::string buildCmd = "cmake --build . --config Release";
        result = std::system(buildCmd.c_str());
        
        // Check if executable was actually created despite return code
        std::string exeName = project->getName();
        #ifdef _WIN32
            exeName += ".exe";
        #endif
        
        bool executableExists = std::filesystem::exists(exeName) || 
                               std::filesystem::exists("Debug/" + exeName) ||
                               std::filesystem::exists("Release/" + exeName);
        
        if (result != 0 && !executableExists) {
            spdlog::error("CMake build failed - no executable generated");
            std::filesystem::current_path(currentDir);
            return false;
        } else if (result != 0 && executableExists) {
            spdlog::warn("CMake build returned non-zero exit code but executable exists - likely due to warnings");
        }
        
        // Restore original directory
        std::filesystem::current_path(currentDir);
        
        // Create output directory
        std::filesystem::create_directories(outputPath);
        
        // Copy executable to output
        // The executable is expected to be named "game" for consistency
        std::string projectName = project->getName();
        #ifdef _WIN32
            std::string builtExeName = projectName + ".exe";
            std::string gameExeName = "game.exe";
        #else
            std::string builtExeName = projectName;
            std::string gameExeName = "game";
        #endif
        
        std::string sourcePath = buildDir + "/" + builtExeName;
        std::string destPath = outputPath + "/" + gameExeName;
        
        try {
            std::filesystem::copy_file(sourcePath, destPath, 
                std::filesystem::copy_options::overwrite_existing);
        } catch (const std::filesystem::filesystem_error& e) {
            // Ignore file exists errors, they're harmless
            if (e.code() != std::errc::file_exists) {
                throw;
            }
        }
        
        // Also copy config and other files to bin directory
        std::string configSource = projectDir + "/game_config.json";
        std::string configDest = outputPath + "/game_config.json";
        if (std::filesystem::exists(configSource)) {
            try {
                std::filesystem::copy_file(configSource, configDest,
                    std::filesystem::copy_options::overwrite_existing);
                
                // Also create a copy with project name for backward compatibility
                std::string projectConfigDest = outputPath + "/" + project->getName() + "_config.json";
                std::filesystem::copy_file(configSource, projectConfigDest,
                    std::filesystem::copy_options::overwrite_existing);
            } catch (const std::filesystem::filesystem_error& e) {
                // Ignore file exists errors, they're harmless
                if (e.code() != std::errc::file_exists) {
                    throw;
                }
            }
        }
        
        // Copy scenes directory to bin (skip if source and dest are the same)
        std::string scenesSource = projectDir + "/scenes";
        std::string scenesDest = outputPath + "/scenes";
        if (std::filesystem::exists(scenesSource) && scenesSource != scenesDest) {
            std::filesystem::create_directories(scenesDest);
            for (const auto& entry : std::filesystem::recursive_directory_iterator(scenesSource)) {
                if (entry.is_regular_file()) {
                    std::string relativePath = std::filesystem::relative(entry.path(), scenesSource).string();
                    std::string destFile = scenesDest + "/" + relativePath;
                    std::filesystem::create_directories(std::filesystem::path(destFile).parent_path());
                    try {
                        std::filesystem::copy_file(entry.path(), destFile,
                            std::filesystem::copy_options::overwrite_existing);
                    } catch (const std::filesystem::filesystem_error& e) {
                        // Ignore file exists errors, they're harmless
                        if (e.code() != std::errc::file_exists) {
                            throw;
                        }
                    }
                }
            }
        }
        
        // Copy assets directory to bin (skip if source and dest are the same)
        std::string assetsSource = projectDir + "/assets";
        std::string assetsDest = outputPath + "/assets";
        if (std::filesystem::exists(assetsSource) && assetsSource != assetsDest) {
            std::filesystem::create_directories(assetsDest);
            for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsSource)) {
                if (entry.is_regular_file()) {
                    std::string relativePath = std::filesystem::relative(entry.path(), assetsSource).string();
                    std::string destFile = assetsDest + "/" + relativePath;
                    std::filesystem::create_directories(std::filesystem::path(destFile).parent_path());
                    try {
                        std::filesystem::copy_file(entry.path(), destFile,
                            std::filesystem::copy_options::overwrite_existing);
                    } catch (const std::filesystem::filesystem_error& e) {
                        // Ignore file exists errors, they're harmless
                        if (e.code() != std::errc::file_exists) {
                            throw;
                        }
                    }
                }
            }
        }
        
        spdlog::info("Project compiled successfully");
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Compilation failed: {}", e.what());
        return false;
    }
}

bool BuildSystem::packageAssets(Project* project, const std::string& outputDir) {
    try {
        std::string assetsSource = project->getPath() + "/assets";
        std::string assetsDest = outputDir + "/assets";
        
        if (std::filesystem::exists(assetsSource)) {
            std::filesystem::create_directories(assetsDest);
            
            // Copy all assets recursively
            for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsSource)) {
                if (entry.is_regular_file()) {
                    std::string relativePath = std::filesystem::relative(entry.path(), assetsSource).string();
                    std::string destPath = assetsDest + "/" + relativePath;
                    
                    // Create parent directories
                    std::filesystem::create_directories(std::filesystem::path(destPath).parent_path());
                    
                    // Copy file
                    try {
                        std::filesystem::copy_file(entry.path(), destPath,
                            std::filesystem::copy_options::overwrite_existing);
                    } catch (const std::filesystem::filesystem_error& e) {
                        // Ignore file exists errors, they're harmless
                        if (e.code() != std::errc::file_exists) {
                            throw;
                        }
                    }
                }
            }
            
            spdlog::info("Assets packaged successfully");
        }
        
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to package assets: {}", e.what());
        return false;
    }
}

bool BuildSystem::createBuildDirectory(const std::string& projectName) {
    try {
        std::filesystem::path buildPath = EnginePaths::getProjectOutputDir(projectName);
        
        // Remove existing build directory if it exists
        if (std::filesystem::exists(buildPath)) {
            std::filesystem::remove_all(buildPath);
        }
        
        // Create directory structure
        std::filesystem::create_directories(buildPath);
        std::filesystem::create_directories(buildPath / "scenes");
        std::filesystem::create_directories(buildPath / "assets");
        std::filesystem::create_directories(buildPath / "bin");
        
        spdlog::info("Created build directory: {}", buildPath.string());
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to create build directory: {}", e.what());
        return false;
    }
}

bool BuildSystem::generateCMakeLists(Project* project, const std::string& outputDir) {
    try {
        // Use the full template that downloads dependencies
        std::filesystem::path templatePath = EnginePaths::getTemplatesDir() / "basic" / "CMakeLists_template.txt";
        
        if (!std::filesystem::exists(templatePath)) {
            // Create a default CMakeLists.txt if template doesn't exist
            std::stringstream cmake;
            cmake << "cmake_minimum_required(VERSION 3.20)\n";
            cmake << "project(" << project->getName() << ")\n\n";
            cmake << "set(CMAKE_CXX_STANDARD 20)\n\n";
            cmake << "# Find packages\n";
            cmake << "find_package(raylib REQUIRED)\n";
            cmake << "find_package(EnTT REQUIRED)\n";
            cmake << "find_package(glm REQUIRED)\n";
            cmake << "find_package(nlohmann_json REQUIRED)\n";
            cmake << "find_package(spdlog REQUIRED)\n\n";
            cmake << "# Add executable\n";
            cmake << "add_executable(" << project->getName() << " main.cpp)\n\n";
            cmake << "# Link libraries\n";
            cmake << "target_link_libraries(" << project->getName() << "\n";
            cmake << "    raylib\n";
            cmake << "    EnTT::EnTT\n";
            cmake << "    glm::glm\n";
            cmake << "    nlohmann_json::nlohmann_json\n";
            cmake << "    spdlog::spdlog\n";
            cmake << ")\n\n";
            cmake << "# Set RPATH for finding libraries\n";
            cmake << "set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)\n\n";
            cmake << "# Copy assets and scenes to build directory\n";
            cmake << "# Remove existing files to avoid copy errors\n";
            cmake << "file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/assets)\n";
            cmake << "file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/scenes)\n";
            cmake << "file(REMOVE ${CMAKE_BINARY_DIR}/game_config.json)\n\n";
            cmake << "# Copy fresh files\n";
            cmake << "file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})\n";
            cmake << "file(COPY ${CMAKE_SOURCE_DIR}/scenes DESTINATION ${CMAKE_BINARY_DIR})\n";
            cmake << "file(COPY ${CMAKE_SOURCE_DIR}/game_config.json DESTINATION ${CMAKE_BINARY_DIR})\n\n";
            cmake << "# Also copy files to the executable directory after build\n";
            cmake << "add_custom_command(TARGET " << project->getName() << " POST_BUILD\n";
            cmake << "    COMMAND ${CMAKE_COMMAND} -E copy_directory\n";
            cmake << "    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:" << project->getName() << ">/assets\n";
            cmake << "    COMMAND ${CMAKE_COMMAND} -E copy_directory\n";
            cmake << "    ${CMAKE_SOURCE_DIR}/scenes $<TARGET_FILE_DIR:" << project->getName() << ">/scenes\n";
            cmake << "    COMMAND ${CMAKE_COMMAND} -E copy\n";
            cmake << "    ${CMAKE_SOURCE_DIR}/game_config.json $<TARGET_FILE_DIR:" << project->getName() << ">/game_config.json\n";
            cmake << "    COMMENT \"Copying game resources to executable directory\"\n";
            cmake << ")\n";
            
            std::ofstream file(outputDir + "/CMakeLists.txt");
            file << cmake.str();
            file.close();
        } else {
            std::string templateContent = FileUtils::readFile(templatePath.string());
            // Make sure the template is using game_config.json
            templateContent = StringUtils::replace(templateContent, "{{PROJECT_NAME}}_config.json", "game_config.json");
            std::string cmakeContent = processTemplate(templateContent, project);
            
            std::ofstream file(outputDir + "/CMakeLists.txt");
            if (!file.is_open()) {
                spdlog::error("Failed to open CMakeLists.txt for writing");
                return false;
            }
            file << cmakeContent;
            file.close();
        }
        
        spdlog::info("Generated CMakeLists.txt");
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to generate CMakeLists.txt: {}", e.what());
        return false;
    }
}

bool BuildSystem::generateCMakeListsFast(Project* project, const std::string& outputDir) {
    try {
        // Use fast template that assumes pre-built dependencies
        std::filesystem::path templatePath = EnginePaths::getTemplatesDir() / "basic" / "CMakeLists_fast.txt";
        
        if (!std::filesystem::exists(templatePath)) {
            // Fall back to regular template
            return generateCMakeLists(project, outputDir);
        }
        
        std::string templateContent = FileUtils::readFile(templatePath.string());
        // Make sure the template is using game_config.json
        templateContent = StringUtils::replace(templateContent, "{{PROJECT_NAME}}_config.json", "game_config.json");
        std::string cmakeContent = processTemplate(templateContent, project);
        
        std::ofstream file(outputDir + "/CMakeLists.txt");
        if (!file.is_open()) {
            spdlog::error("Failed to open CMakeLists.txt for writing");
            return false;
        }
        file << cmakeContent;
        file.close();
        
        spdlog::info("Generated CMakeLists.txt (fast mode)");
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to generate CMakeLists.txt: {}", e.what());
        return false;
    }
}

bool BuildSystem::copyRuntimeLibrary(const std::string& outputDir) {
    // For now, we'll embed the runtime directly in the generated code
    // In a full implementation, this would copy pre-compiled runtime libraries
    spdlog::info("Runtime library embedded in generated code");
    return true;
}

bool BuildSystem::processScenes(Project* project, const std::string& outputDir) {
    try {
        std::string scenesSource = project->getPath() + "/scenes";
        std::string scenesDest = outputDir + "/scenes";
        
        if (std::filesystem::exists(scenesSource)) {
            std::filesystem::create_directories(scenesDest);
            
            // Copy all scene files
            for (const auto& entry : std::filesystem::directory_iterator(scenesSource)) {
                if (entry.path().extension() == ".json") {
                    try {
                        std::filesystem::copy_file(entry.path(), 
                            scenesDest + "/" + entry.path().filename().string(),
                            std::filesystem::copy_options::overwrite_existing);
                    } catch (const std::filesystem::filesystem_error& e) {
                        // Ignore file exists errors during copy
                        if (e.code() != std::errc::file_exists) {
                            throw;
                        }
                    }
                }
            }
            
            spdlog::info("Scenes processed successfully");
        }
        
        // Create game config file
        nlohmann::json gameConfig;
        gameConfig["name"] = project->getName();
        gameConfig["version"] = "1.0.0";
        gameConfig["main_scene"] = project->getScenes().empty() ? "main_scene" : project->getScenes()[0];
        
        // Remove existing config file if it exists to avoid copy errors
        std::string configPath = outputDir + "/game_config.json";
        if (std::filesystem::exists(configPath)) {
            std::filesystem::remove(configPath);
        }
        
        std::ofstream configFile(configPath);
        configFile << gameConfig.dump(4);
        configFile.close();
        
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("Failed to process scenes: {}", e.what());
        return false;
    }
}

std::string BuildSystem::processTemplate(const std::string& templateContent, Project* project) {
    std::string result = templateContent;
    
    // Replace template variables
    result = StringUtils::replace(result, "{{PROJECT_NAME}}", project->getName());
    
    // Get main scene
    std::string mainScene = "main_scene";
    if (!project->getScenes().empty()) {
        mainScene = project->getScenes()[0];
    }
    result = StringUtils::replace(result, "{{MAIN_SCENE}}", mainScene);
    
    return result;
}

} // namespace GameEngine