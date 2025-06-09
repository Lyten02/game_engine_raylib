#include "command_registry.h"
#include "../console/console.h"
#include "../console/command_processor.h"
#include "../project/project_manager.h"
#include "../project/project.h"
#include "../build/build_system.h"
#include "../build/async_build_system.h"
#include "../engine/play_mode.h"
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
            
            console->addLine("Building project: " + projectManager->getCurrentProject()->getName() + "...", YELLOW);
            
            // Check for --test flag to skip actual compilation
            bool testMode = false;
            for (const auto& arg : args) {
                if (arg == "--test") {
                    testMode = true;
                    break;
                }
            }
            
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
    
    // project.build-fast command for testing
    processor->registerCommand("project.build-fast",
        [console, projectManager, buildSystem](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project open. Use 'project.open' first.", RED);
                return;
            }
            
            console->addLine("Fast build (no compilation): " + projectManager->getCurrentProject()->getName() + "...", YELLOW);
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
                console->addLine("Build preparation completed!", GREEN);
                console->addLine("Generated files in: " + outputDir, GRAY);
                
                // Set success message for CLI
                if (console->isCaptureMode()) {
                    console->addLine("Build completed successfully", GREEN);
                }
            } else {
                console->addLine("Build preparation failed!", RED);
            }
        }, "Fast build without compilation (for testing)", "Build");
    
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
            
            // Run in background
            #ifdef _WIN32
                std::string command = "start \"\" \"" + execPath + "\"";
            #else
                std::string command = "\"" + execPath + "\" &";
            #endif
            
            int result = std::system(command.c_str());
            if (result == 0) {
                console->addLine("Game launched successfully", GREEN);
            } else {
                console->addLine("Failed to launch game", RED);
            }
        }, "Run the built executable", "Build");
    
    // project.build.async command  
    processor->registerCommand("project.build.async",
        [console, projectManager, asyncBuildSystem](const std::vector<std::string>& args) {
            if (!projectManager->getCurrentProject()) {
                console->addLine("No project open. Use 'project.open' first.", RED);
                return;
            }
            
            if (asyncBuildSystem->getStatus() == AsyncBuildSystem::BuildStatus::InProgress) {
                console->addLine("Build already in progress", YELLOW);
                return;
            }
            
            console->addLine("Starting async build for: " + projectManager->getCurrentProject()->getName(), YELLOW);
            if (!asyncBuildSystem->startBuild(projectManager->getCurrentProject())) {
                console->addLine("Failed to start build - another build may be in progress", RED);
            }
        }, "Build project asynchronously", "Build");
}

} // namespace GameEngine