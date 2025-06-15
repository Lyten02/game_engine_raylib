#include "command_registry.h"
#include "../console/console.h"
#include "../console/command_processor.h"
#include "../project/project_manager.h"
#include "../project/project.h"
#include "../build/build_system.h"
#include "../build/async_build_system.h"
#include "../engine/play_mode.h"
#include "../utils/log_limiter.h"
#include "../utils/engine_paths.h"
#include "../utils/process_executor.h"
#include <raylib.h>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <cstdlib>
#include <sstream>

namespace GameEngine {

void CommandRegistry::registerBuildCommands(CommandProcessor* processor, Console* console, 
                                          ProjectManager* projectManager,
                                          BuildSystem* buildSystem, 
                                          AsyncBuildSystem* asyncBuildSystem) {
    // project.build command
    processor->registerCommand("project.build",
        [console, projectManager, buildSystem](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project open. Use 'project.open' first.", RED);
                return;
            }
            
            // Check for --test flag to skip actual compilation
            bool testMode = false;
            for (const auto& arg : args) {
                if (arg == "--test") {
                    testMode = true;
                    break;
                }
            }
            
            // Use log limiter for build messages in test mode
            std::string buildMsg = "Building project: " + projectManager->getCurrentProject()->getName() + "...";
            if (testMode) {
                LogLimiter::info("building_project", "Building project: {}", projectManager->getCurrentProject()->getName());
            }
            console->addLine(buildMsg, YELLOW);
            
            if (testMode) {
                // In test mode, just generate files without compiling
                console->addLine("Test mode: Generating build files only...", YELLOW);
                std::string projectName = projectManager->getCurrentProject()->getName();
                std::string outputDir = "output/" + projectName;
                
                // Create build directory
                if (!buildSystem->createBuildDirectory(projectName)) {
                    console->addLine("Failed to create build directory!", RED);
                    return;
                }
                
                bool success = buildSystem->generateGameCode(projectManager->getCurrentProject(), outputDir);
                if (success) {
                    buildSystem->generateCMakeLists(projectManager->getCurrentProject(), outputDir);
                    buildSystem->processScenes(projectManager->getCurrentProject(), outputDir);
                    buildSystem->packageAssets(projectManager->getCurrentProject(), outputDir);
                    console->addLine("Build preparation completed!", GREEN);
                    console->addLine("Generated files in: " + outputDir, GRAY);
                } else {
                    console->addLine("Build preparation failed!", RED);
                }
            } else {
                // Normal build with compilation
                bool success = buildSystem->buildProject(projectManager->getCurrentProject());
                if (success) {
                    console->addLine("Build succeeded!", GREEN);
                    std::string execPath = "output/" + projectManager->getCurrentProject()->getName() + "/game";
                    console->addLine("Executable: " + execPath, GRAY);
                } else {
                    console->addLine("Build failed!", RED);
                }
            }
        }, "Build the current project", "Build");
    
    // project.prepare command - prepares project files without compilation
    processor->registerCommand("project.prepare",
        [console, projectManager, buildSystem](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project open. Use 'project.open' first.", RED);
                return;
            }
            
            console->addLine("Preparing project files: " + projectManager->getCurrentProject()->getName() + "...", YELLOW);
            std::string projectName = projectManager->getCurrentProject()->getName();
            std::string outputDir = "output/" + projectName;
            
            // Create build directory
            if (!buildSystem->createBuildDirectory(projectName)) {
                console->addLine("Failed to create build directory!", RED);
                return;
            }
            
            bool success = buildSystem->generateGameCode(projectManager->getCurrentProject(), outputDir);
            if (success) {
                buildSystem->generateCMakeListsFast(projectManager->getCurrentProject(), outputDir);
                buildSystem->processScenes(projectManager->getCurrentProject(), outputDir);
                buildSystem->packageAssets(projectManager->getCurrentProject(), outputDir);
                console->addLine("Project preparation completed!", GREEN);
                console->addLine("Generated files in: " + outputDir, GRAY);
                
                // Set success message for CLI
                if (console->isCaptureMode()) {
                    console->addLine("Project prepared successfully", GREEN);
                }
            } else {
                console->addLine("Project preparation failed!", RED);
            }
        }, "Prepare project files without compilation", "Build");
    
    // project.clean command
    processor->registerCommand("project.clean",
        [console, projectManager, buildSystem](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project open. Use 'project.open' first.", RED);
                return;
            }
            
            console->addLine("Cleaning project: " + projectManager->getCurrentProject()->getName() + "...", YELLOW);
            
            // Clean is not implemented in BuildSystem
            std::string outputPath = "output/" + projectManager->getCurrentProject()->getName();
            if (std::filesystem::exists(outputPath)) {
                try {
                    std::filesystem::remove_all(outputPath);
                    console->addLine("Project cleaned successfully", GREEN);
                } catch (const std::exception& e) {
                    console->addLine("Failed to clean project: " + std::string(e.what()), RED);
                }
            } else {
                console->addLine("Nothing to clean", YELLOW);
            }
        }, "Clean build artifacts", "Build");
    
    // project.rebuild command
    processor->registerCommand("project.rebuild",
        [console, projectManager, buildSystem](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project open. Use 'project.open' first.", RED);
                return;
            }
            
            console->addLine("Rebuilding project: " + projectManager->getCurrentProject()->getName() + "...", YELLOW);
            
            // Clean first
            std::string outputPath = "output/" + projectManager->getCurrentProject()->getName();
            if (std::filesystem::exists(outputPath)) {
                try {
                    std::filesystem::remove_all(outputPath);
                } catch (...) {}
            }
            
            // Then build
            bool success = buildSystem->buildProject(projectManager->getCurrentProject());
            if (success) {
                console->addLine("Rebuild succeeded!", GREEN);
                std::string execPath = outputPath + "/game";
                console->addLine("Executable: " + execPath, GRAY);
            } else {
                console->addLine("Rebuild failed!", RED);
            }
        }, "Clean and rebuild project", "Build");
    
    // project.run command
    processor->registerCommand("project.run",
        [console, projectManager](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project open. Use 'project.open' first.", RED);
                return;
            }
            
            std::string execPath = "output/" + projectManager->getCurrentProject()->getName() + "/game";
            
            if (!std::filesystem::exists(execPath)) {
                console->addLine("Executable not found. Build the project first.", RED);
                return;
            }
            
            console->addLine("Running: " + execPath, YELLOW);
            
            // Use ProcessExecutor for safe execution
            ProcessExecutor executor;
            
            // Get the directory containing the executable
            std::filesystem::path execFullPath = std::filesystem::absolute(execPath);
            std::string workingDir = execFullPath.parent_path().string();
            
            // Execute the game safely without shell interpretation
            auto result = executor.execute(execFullPath.string(), {}, workingDir);
            
            if (result.exitCode == 0 || result.exitCode == -2) {  // -2 means process launched in background
                console->addLine("Game launched successfully", GREEN);
            } else {
                console->addLine("Failed to launch game. Exit code: " + std::to_string(result.exitCode), RED);
                if (!result.error.empty()) {
                    console->addLine("Error: " + result.error, RED);
                }
            }
        }, "Run the built executable", "Build");
    
    // project.build.fast command - fast build with pre-compiled dependencies
    processor->registerCommand("project.build.fast",
        [console, projectManager, buildSystem](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project open. Use 'project.open' first.", RED);
                return;
            }
            
            console->addLine("Fast build with cached dependencies: " + projectManager->getCurrentProject()->getName() + "...", YELLOW);
            std::string projectName = projectManager->getCurrentProject()->getName();
            std::string outputDir = EnginePaths::getProjectOutputDir(projectName).string();
            
            // Check if main project dependencies exist
            // Use centralized path system
            std::filesystem::path mainBuildDir = EnginePaths::getBuildDir();
            std::filesystem::path depsDir = EnginePaths::getDependenciesDir();
            
            // Check if dependencies exist
            if (!std::filesystem::exists(depsDir)) {
                console->addLine("Cannot find main build directory with dependencies.", RED);
                console->addLine("Expected at: " + depsDir.string(), GRAY);
                console->addLine("Make sure you've run 'make' in the " + mainBuildDir.string() + " directory first.", YELLOW);
                return;
            }
            
            std::filesystem::path depsPath = depsDir / "raylib-build/raylib/libraylib.a";
            if (!std::filesystem::exists(depsPath)) {
                console->addLine("Cached dependencies not found. Run a full build first.", RED);
                console->addLine("Missing: " + depsPath.string(), GRAY);
                console->addLine("Dependencies directory: " + depsDir.string(), GRAY);
                return;
            }
            
            // Create build directory
            if (!buildSystem->createBuildDirectory(projectName)) {
                console->addLine("Failed to create build directory!", RED);
                return;
            }
            
            // Generate game code
            if (!buildSystem->generateGameCode(projectManager->getCurrentProject(), outputDir)) {
                console->addLine("Failed to generate game code!", RED);
                return;
            }
            
            // Generate fast CMakeLists
            if (!buildSystem->generateCMakeListsFast(projectManager->getCurrentProject(), outputDir)) {
                console->addLine("Failed to generate CMakeLists.txt!", RED);
                return;
            }
            
            // Process scenes and assets
            buildSystem->processScenes(projectManager->getCurrentProject(), outputDir);
            buildSystem->packageAssets(projectManager->getCurrentProject(), outputDir);
            
            // Compile with fast build
            bool success = buildSystem->compileProject(projectManager->getCurrentProject(), outputDir, outputDir);
            if (success) {
                console->addLine("Fast build succeeded!", GREEN);
                std::string execPath = outputDir + "/game";
                console->addLine("Executable: " + execPath, GRAY);
            } else {
                console->addLine("Fast build failed!", RED);
            }
        }, "Build project using cached dependencies (fast)", "Build");
    
    // project.build.async command  
    processor->registerCommand("project.build.async",
        [console, projectManager, asyncBuildSystem](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project open. Use 'project.open' first.", RED);
                return;
            }
            
            if (asyncBuildSystem->getStatus() == AsyncBuildSystem::BuildStatus::InProgress) {
                // Use log limiter to prevent spam
                if (LogLimiter::shouldLog("build_in_progress")) {
                    console->addLine("Build already in progress", YELLOW);
                }
                return;
            }
            
            console->addLine("Starting async build for: " + projectManager->getCurrentProject()->getName(), YELLOW);
            if (!asyncBuildSystem->startBuild(projectManager->getCurrentProject())) {
                console->addLine("Failed to start build - another build may be in progress", RED);
            }
        }, "Build project asynchronously", "Build");
    
    // project.build.async.fast command - async fast build with cached dependencies
    processor->registerCommand("project.build.async.fast",
        [console, projectManager, asyncBuildSystem](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project open. Use 'project.open' first.", RED);
                return;
            }
            
            if (asyncBuildSystem->getStatus() == AsyncBuildSystem::BuildStatus::InProgress) {
                // Use log limiter to prevent spam
                if (LogLimiter::shouldLog("build_in_progress")) {
                    console->addLine("Build already in progress", YELLOW);
                }
                return;
            }
            
            console->addLine("Starting async fast build for: " + projectManager->getCurrentProject()->getName(), YELLOW);
            if (!asyncBuildSystem->startFastBuild(projectManager->getCurrentProject())) {
                console->addLine("Failed to start fast build - another build may be in progress", RED);
            }
        }, "Build project asynchronously with cached dependencies (fastest)", "Build");
}

} // namespace GameEngine