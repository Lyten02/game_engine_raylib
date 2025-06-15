#include "async_build_system.h"
#include "build_system.h"
#include "../project/project.h"
#include "../utils/log_limiter.h"
#include "../utils/engine_paths.h"
#include "../utils/process_executor.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <regex>
#include <fstream>

namespace GameEngine {

AsyncBuildSystem::AsyncBuildSystem() {
    buildSystem = std::make_unique<BuildSystem>();
}

AsyncBuildSystem::~AsyncBuildSystem() {
    cancelBuild();
}

bool AsyncBuildSystem::startBuild(Project* project, const std::string& buildConfig) {
    // Lock for thread-safe access to buildThread
    std::lock_guard<std::mutex> threadLock(buildThreadMutex);
    
    // Check if previous build thread needs to be cleaned up
    if (buildThread && buildThread->joinable()) {
        BuildStatus currentStatus = buildProgress.status.load();
        if (currentStatus == BuildStatus::Success || currentStatus == BuildStatus::Failed) {
            // Previous build finished, clean it up
            buildThread->join();
            buildThread.reset();
            buildProgress.status.store(BuildStatus::Idle);
        }
    }
    
    // Use atomic compare-and-swap to avoid race condition
    BuildStatus expected = BuildStatus::Idle;
    if (!buildProgress.status.compare_exchange_strong(expected, BuildStatus::InProgress)) {
        // Another build is already in progress
        addMessageWithLimit("build_in_progress", "Build already in progress!");
        return false;
    }
    
    // Successfully changed status from Idle to InProgress
    // Reset progress
    buildProgress.progress.store(0.0f);
    buildProgress.errorMessage.clear();
    
    // Clear currentStep
    {
        std::lock_guard<std::mutex> lock(buildProgress.currentStepMutex);
        buildProgress.currentStep.clear();
    }
    
    // Clear message queue
    {
        std::lock_guard<std::mutex> lock(buildProgress.messageMutex);
        std::queue<std::string> empty;
        std::swap(buildProgress.messages, empty);
    }
    
    // Start build thread
    if (buildThread && buildThread->joinable()) {
        buildThread->join();
    }
    
    buildThread = std::make_unique<std::thread>(
        &AsyncBuildSystem::buildThreadFunc, this, project, buildConfig
    );
    
    return true;
}

bool AsyncBuildSystem::startFastBuild(Project* project, const std::string& buildConfig) {
    // Lock for thread-safe access to buildThread
    std::lock_guard<std::mutex> threadLock(buildThreadMutex);
    
    // Check if previous build thread needs to be cleaned up
    if (buildThread && buildThread->joinable()) {
        BuildStatus currentStatus = buildProgress.status.load();
        if (currentStatus == BuildStatus::Success || currentStatus == BuildStatus::Failed) {
            // Previous build finished, clean it up
            buildThread->join();
            buildThread.reset();
            buildProgress.status.store(BuildStatus::Idle);
        }
    }
    
    // Use atomic compare-and-swap to avoid race condition
    BuildStatus expected = BuildStatus::Idle;
    if (!buildProgress.status.compare_exchange_strong(expected, BuildStatus::InProgress)) {
        // Another build is already in progress
        addMessageWithLimit("build_in_progress", "Build already in progress!");
        return false;
    }
    
    // Successfully changed status from Idle to InProgress
    // Reset progress
    buildProgress.progress.store(0.0f);
    buildProgress.errorMessage.clear();
    
    // Clear currentStep
    {
        std::lock_guard<std::mutex> lock(buildProgress.currentStepMutex);
        buildProgress.currentStep.clear();
    }
    
    // Clear message queue
    {
        std::lock_guard<std::mutex> lock(buildProgress.messageMutex);
        std::queue<std::string> empty;
        std::swap(buildProgress.messages, empty);
    }
    
    // Start build thread
    if (buildThread && buildThread->joinable()) {
        buildThread->join();
    }
    
    buildThread = std::make_unique<std::thread>(
        &AsyncBuildSystem::fastBuildThreadFunc, this, project, buildConfig
    );
    
    return true;
}

void AsyncBuildSystem::cancelBuild() {
    std::lock_guard<std::mutex> lock(buildThreadMutex);
    if (buildThread && buildThread->joinable()) {
        // In a real implementation, we'd have a cancellation mechanism
        // For now, just wait for it to finish
        buildThread->join();
    }
}

bool AsyncBuildSystem::hasMessages() {
    std::lock_guard<std::mutex> lock(buildProgress.messageMutex);
    return !buildProgress.messages.empty();
}

std::string AsyncBuildSystem::getNextMessage() {
    std::lock_guard<std::mutex> lock(buildProgress.messageMutex);
    if (buildProgress.messages.empty()) {
        return "";
    }
    std::string msg = buildProgress.messages.front();
    buildProgress.messages.pop();
    return msg;
}

void AsyncBuildSystem::waitForCompletion() {
    std::lock_guard<std::mutex> lock(buildThreadMutex);
    if (buildThread && buildThread->joinable()) {
        buildThread->join();
    }
}

void AsyncBuildSystem::buildThreadFunc(Project* project, const std::string& buildConfig) {
    try {
        setProgress(0.0f, "Starting build...");
        addMessage("Building project: " + project->getName());
        
        // Create build directory
        setProgress(0.1f, "Creating build directory...");
        std::string projectName = project->getName();
        
        // Use centralized path system
        std::filesystem::path buildDir = EnginePaths::getProjectOutputDir(projectName);
        
        // Simulate the build steps with progress updates
        std::filesystem::create_directories(buildDir);
        addMessage("Created build directory: " + buildDir.string());
        
        // Generate game code
        setProgress(0.2f, "Generating game code...");
        if (!buildSystem->generateGameCode(project, buildDir.string())) {
            throw std::runtime_error("Failed to generate game code");
        }
        addMessage("Generated main.cpp");
        
        // Generate CMakeLists
        setProgress(0.3f, "Generating CMakeLists.txt...");
        if (!buildSystem->generateCMakeLists(project, buildDir.string())) {
            throw std::runtime_error("Failed to generate CMakeLists.txt");
        }
        addMessage("Generated CMakeLists.txt");
        
        // Process scenes
        setProgress(0.4f, "Processing scenes...");
        if (!buildSystem->processScenes(project, buildDir.string())) {
            throw std::runtime_error("Failed to process scenes");
        }
        addMessage("Processed scene files");
        
        // Package assets
        setProgress(0.5f, "Packaging assets...");
        if (!buildSystem->packageAssets(project, buildDir.string())) {
            throw std::runtime_error("Failed to package assets");
        }
        addMessage("Packaged game assets");
        
        // Configure with CMake
        setProgress(0.6f, "Configuring with CMake...");
        std::filesystem::path cmakeBuildDir = buildDir / "build";
        std::filesystem::create_directories(cmakeBuildDir);
        
        std::string currentDir = std::filesystem::current_path().string();
        
        try {
            std::filesystem::current_path(cmakeBuildDir);
            
            addMessage("Running CMake configuration...");
            std::vector<std::string> cmakeArgs = {"-DCMAKE_BUILD_TYPE=" + buildConfig, ".."};
            
            auto result = ProcessExecutor::executeStreaming(
                "cmake", cmakeArgs, cmakeBuildDir.string(),
                [this](const std::string& line) {
                    // Filter out some verbose CMake output
                    if (line.find("-- ") == 0 || line.find("CMake") != std::string::npos) {
                        addMessage(line);
                    }
                }
            );
            
            if (!result.success) {
                std::filesystem::current_path(currentDir);
                throw std::runtime_error("CMake configuration failed: " + result.error);
            }
        } catch (...) {
            std::filesystem::current_path(currentDir);
            throw;
        }
        
        // Build with CMake
        setProgress(0.7f, "Compiling project...");
        addMessage("Starting compilation...");
        
        try {
            std::vector<std::string> buildArgs = {"--build", ".", "--config", buildConfig};
            std::regex progressRegex(R"(\[\s*(\d+)%\])");
            
            auto buildResult = ProcessExecutor::executeStreaming(
                "cmake", buildArgs, cmakeBuildDir.string(),
                [this, &progressRegex](const std::string& line) {
                    // Parse build progress
                    std::smatch match;
                    if (std::regex_search(line, match, progressRegex)) {
                        int percent = std::stoi(match[1]);
                        float buildProgress = 0.7f + (percent / 100.0f) * 0.2f;
                        setProgress(buildProgress, "Compiling... " + std::to_string(percent) + "%");
                    }
                    
                    // Show important messages
                    if (line.find("Building") != std::string::npos ||
                        line.find("Linking") != std::string::npos ||
                        line.find("Built target") != std::string::npos) {
                        addMessage(line);
                    }
                }
            );
            
            if (!buildResult.success) {
                std::filesystem::current_path(currentDir);
                throw std::runtime_error("Build failed: " + buildResult.error);
            }
            
            std::filesystem::current_path(currentDir);
        } catch (...) {
            // Ensure we restore directory even on exception
            try { std::filesystem::current_path(currentDir); } catch (...) {}
            throw;
        }
        
        // Copy executable and resources
        setProgress(0.9f, "Copying executable...");
        if (!buildSystem->compileProject(project, buildDir.string(), (buildDir / "bin").string())) {
            throw std::runtime_error("Failed to finalize build");
        }
        
        setProgress(1.0f, "Build completed!");
        addMessage("Build completed successfully!");
        addMessage("Output: " + (buildDir / "bin" / projectName).string());
        
        buildProgress.status.store(BuildStatus::Success);
    }
    catch (const std::exception& e) {
        buildProgress.errorMessage = e.what();
        addMessage("Build failed: " + std::string(e.what()));
        buildProgress.status.store(BuildStatus::Failed);
        spdlog::error("Build failed: {}", e.what());
    }
}

void AsyncBuildSystem::addMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(buildProgress.messageMutex);
    buildProgress.messages.push(message);
    spdlog::info("[Build] {}", message);
}

void AsyncBuildSystem::addMessageWithLimit(const std::string& key, const std::string& message) {
    std::lock_guard<std::mutex> lock(buildProgress.messageMutex);
    buildProgress.messages.push(message);
    LogLimiter::info(key, "[Build] {}", message);
}

void AsyncBuildSystem::setProgress(float progress, const std::string& step) {
    buildProgress.progress.store(progress);
    {
        std::lock_guard<std::mutex> lock(buildProgress.currentStepMutex);
        buildProgress.currentStep = step;
    }
}

void AsyncBuildSystem::fastBuildThreadFunc(Project* project, const std::string& buildConfig) {
    try {
        setProgress(0.0f, "Starting fast build...");
        addMessage("Fast build with cached dependencies: " + project->getName());
        
        std::string projectName = project->getName();
        // Use centralized path system
        std::filesystem::path outputDir = EnginePaths::getProjectOutputDir(projectName);
        
        // Check if main project dependencies exist
        setProgress(0.1f, "Checking cached dependencies...");
        
        // Use centralized path system
        std::filesystem::path mainBuildDir = EnginePaths::getBuildDir();
        std::filesystem::path depsDir = EnginePaths::getDependenciesDir();
        
        if (!std::filesystem::exists(depsDir)) {
            throw std::runtime_error("Cannot find main build directory with dependencies at: " + 
                                   depsDir.string() + ". Run 'make' in the " + mainBuildDir.string() + " directory first.");
        }
        
        std::string depsPath = (depsDir / "raylib-build/raylib/libraylib.a").string();
        if (!std::filesystem::exists(depsPath)) {
            throw std::runtime_error("Cached dependencies not found. Run a full build first.");
        }
        
        addMessage("Found cached dependencies at: " + depsDir.string());
        
        // Create build directory
        setProgress(0.2f, "Creating build directory...");
        std::filesystem::create_directories(outputDir);
        addMessage("Created build directory: " + outputDir.string());
        
        // Generate game code
        setProgress(0.3f, "Generating game code...");
        if (!buildSystem->generateGameCode(project, outputDir.string())) {
            throw std::runtime_error("Failed to generate game code");
        }
        addMessage("Generated main.cpp");
        
        // Generate fast CMakeLists
        setProgress(0.4f, "Generating CMakeLists.txt (fast mode)...");
        if (!buildSystem->generateCMakeListsFast(project, outputDir.string())) {
            throw std::runtime_error("Failed to generate CMakeLists.txt");
        }
        addMessage("Generated CMakeLists.txt with cached dependencies");
        
        // Process scenes and assets
        setProgress(0.5f, "Processing scenes...");
        buildSystem->processScenes(project, outputDir.string());
        addMessage("Processed scene files");
        
        setProgress(0.6f, "Packaging assets...");
        buildSystem->packageAssets(project, outputDir.string());
        addMessage("Packaged game assets");
        
        // Configure with CMake
        setProgress(0.7f, "Configuring with CMake...");
        std::filesystem::path cmakeBuildDir = outputDir / "build";
        std::filesystem::create_directories(cmakeBuildDir);
        
        std::string currentDir = std::filesystem::current_path().string();
        try {
            std::filesystem::current_path(cmakeBuildDir);
            
            addMessage("Running CMake configuration (fast mode)...");
            std::vector<std::string> cmakeArgs = {"-DCMAKE_BUILD_TYPE=" + buildConfig, ".."};
            
            auto result = ProcessExecutor::executeStreaming(
                "cmake", cmakeArgs, cmakeBuildDir.string(),
                [this](const std::string& line) {
                    if (line.find("-- ") == 0 || line.find("Found cached") != std::string::npos) {
                        addMessage(line);
                    }
                }
            );
            
            if (!result.success) {
                std::filesystem::current_path(currentDir);
                throw std::runtime_error("CMake configuration failed: " + result.error);
            }
        } catch (...) {
            std::filesystem::current_path(currentDir);
            throw;
        }
        
        // Build with CMake (much faster with cached dependencies)
        setProgress(0.8f, "Compiling project (fast mode)...");
        addMessage("Starting compilation with cached dependencies...");
        
        try {
            std::vector<std::string> buildArgs = {"--build", ".", "--config", buildConfig};
            std::regex progressRegex(R"(\[\s*(\d+)%\])");
            
            auto buildResult = ProcessExecutor::executeStreaming(
                "cmake", buildArgs, cmakeBuildDir.string(),
                [this, &progressRegex](const std::string& line) {
                    // Parse build progress
                    std::smatch match;
                    if (std::regex_search(line, match, progressRegex)) {
                        int percent = std::stoi(match[1]);
                        float buildProgress = 0.8f + (percent / 100.0f) * 0.15f;
                        setProgress(buildProgress, "Compiling (fast)... " + std::to_string(percent) + "%");
                    }
                    
                    // Show important messages
                    if (line.find("Building") != std::string::npos ||
                        line.find("Linking") != std::string::npos ||
                        line.find("Built target") != std::string::npos) {
                        addMessage(line);
                    }
                }
            );
            
            if (!buildResult.success) {
                std::filesystem::current_path(currentDir);
                throw std::runtime_error("Build failed: " + buildResult.error);
            }
            
            std::filesystem::current_path(currentDir);
        } catch (...) {
            // Ensure we restore directory even on exception
            try { std::filesystem::current_path(currentDir); } catch (...) {}
            throw;
        }
        
        // Finalize
        setProgress(0.95f, "Finalizing build...");
        std::filesystem::path execPath = outputDir / projectName;
        if (std::filesystem::exists(cmakeBuildDir / projectName)) {
            try {
                std::filesystem::copy_file(
                    cmakeBuildDir / projectName, 
                    execPath,
                    std::filesystem::copy_options::overwrite_existing
                );
            } catch (const std::filesystem::filesystem_error& e) {
                // Ignore file exists errors, they're harmless
                if (e.code() != std::errc::file_exists) {
                    throw;
                }
            }
            std::filesystem::permissions(execPath, 
                std::filesystem::perms::owner_exec | 
                std::filesystem::perms::group_exec | 
                std::filesystem::perms::others_exec,
                std::filesystem::perm_options::add
            );
        }
        
        setProgress(1.0f, "Fast build completed!");
        addMessage("Fast build completed successfully!");
        addMessage("Executable: " + execPath.string());
        
        buildProgress.status.store(BuildStatus::Success);
    }
    catch (const std::exception& e) {
        buildProgress.errorMessage = e.what();
        addMessage("Fast build failed: " + std::string(e.what()));
        buildProgress.status.store(BuildStatus::Failed);
        spdlog::error("Fast build failed: {}", e.what());
    }
}

} // namespace GameEngine