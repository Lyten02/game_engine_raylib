#include "engine_paths.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <iostream>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#elif _WIN32
#include <windows.h>
#elif __linux__
#include <unistd.h>
#include <linux/limits.h>
#endif

namespace GameEngine {
// Static member definitions
std::filesystem::path EnginePaths::engineRoot;
bool EnginePaths::initialized = false;

void EnginePaths::initialize() {
    if (initialized) return;

    // Get executable path
    std::filesystem::path execPath;

#ifdef __APPLE__
        char pathBuf[PATH_MAX];
        uint32_t size = sizeof(pathBuf);
        if (_NSGetExecutablePath(pathBuf, &size) == 0) {
            execPath = std::filesystem::canonical(pathBuf);
        } else {
            execPath = std::filesystem::current_path() / "game";
        }
#elif _WIN32
        char pathBuf[MAX_PATH];
        GetModuleFileName(NULL, pathBuf, MAX_PATH);
        execPath = std::filesystem::canonical(pathBuf);
#elif __linux__
        char pathBuf[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", pathBuf, PATH_MAX);
        if (count != -1) {
            pathBuf[count] = '\0';
            execPath = std::filesystem::canonical(pathBuf);
        } else {
            execPath = std::filesystem::current_path() / "game";
        }
#else
        execPath = std::filesystem::current_path() / "game";
#endif

    // Determine engine root
    // If we're in build directory, go up one level
    engineRoot = execPath.parent_path();
    if (engineRoot.filename() == "build") {
        engineRoot = engineRoot.parent_path();
    }

    // Ensure engine root is absolute
    engineRoot = std::filesystem::absolute(engineRoot);

    initialized = true;

    spdlog::info("EnginePaths initialized with root: {}", engineRoot.string());
}

void EnginePaths::ensureInitialized() {
    if (!initialized) {
        initialize();
    }
}

std::filesystem::path EnginePaths::getEngineRoot() {
    ensureInitialized();
    return engineRoot;
}

std::filesystem::path EnginePaths::getProjectsDir() {
    ensureInitialized();
    return engineRoot / "projects";
}

std::filesystem::path EnginePaths::getOutputDir() {
    ensureInitialized();
    return engineRoot / "output";
}

std::filesystem::path EnginePaths::getBuildDir() {
    ensureInitialized();
    return engineRoot / "build";
}

std::filesystem::path EnginePaths::getDependenciesDir() {
    ensureInitialized();
    // Check for global deps cache first
    std::filesystem::path globalCache = engineRoot / ".deps_cache" / "_deps";
    if (std::filesystem::exists(globalCache)) {
        return globalCache;
    }
    // Fallback to build directory deps
    return engineRoot / "build" / "_deps";
}

std::filesystem::path EnginePaths::getTemplatesDir() {
    ensureInitialized();
    return engineRoot / "templates";
}

std::filesystem::path EnginePaths::getLogsDir() {
    ensureInitialized();
    return engineRoot / "logs";
}

std::filesystem::path EnginePaths::getConfigFile() {
    ensureInitialized();
    return engineRoot / "config.json";
}

std::filesystem::path EnginePaths::getPackagesDir() {
    ensureInitialized();
    return engineRoot / "packages";
}

std::filesystem::path EnginePaths::getProjectDir(const std::string& projectName) {
    ensureInitialized();
    return engineRoot / "projects" / projectName;
}

std::filesystem::path EnginePaths::getProjectOutputDir(const std::string& projectName) {
    ensureInitialized();
    return engineRoot / "output" / projectName;
}

std::filesystem::path EnginePaths::getProjectBuildDir(const std::string& projectName) {
    ensureInitialized();
    return engineRoot / "output" / projectName / "build";
}

std::filesystem::path EnginePaths::makeAbsolute(const std::filesystem::path& relativePath) {
    ensureInitialized();
    if (relativePath.is_absolute()) {
        return relativePath;
    }
    return engineRoot / relativePath;
}

std::filesystem::path EnginePaths::makeRelative(const std::filesystem::path& absolutePath) {
    ensureInitialized();
    try {
        return std::filesystem::relative(absolutePath, engineRoot);
    } catch (...) {
        return absolutePath;
    }
}

void EnginePaths::displayPaths() {
    ensureInitialized();

    std::cout << "========================================\n";
    std::cout << "Engine Paths Information:\n";
    std::cout << "========================================\n";

    auto cwd = std::filesystem::current_path();

    std::cout << "Current Working Directory:\n";
    std::cout << "  Absolute: " << cwd.string() << "\n";

    std::cout << "Engine Root:\n";
    std::cout << "  Relative: " << makeRelative(engineRoot).string() << "\n";
    std::cout << "  Absolute: " << engineRoot.string() << "\n";

    auto projectsDir = getProjectsDir();
    std::cout << "Projects Directory:\n";
    std::cout << "  Relative: " << makeRelative(projectsDir).string() << "\n";
    std::cout << "  Absolute: " << projectsDir.string() << "\n";
    std::cout << "  Exists: " << (std::filesystem::exists(projectsDir) ? "Yes" : "No") << "\n";

    auto outputDir = getOutputDir();
    std::cout << "Output Directory:\n";
    std::cout << "  Relative: " << makeRelative(outputDir).string() << "\n";
    std::cout << "  Absolute: " << outputDir.string() << "\n";
    std::cout << "  Exists: " << (std::filesystem::exists(outputDir) ? "Yes" : "No") << "\n";

    auto buildDir = getBuildDir();
    std::cout << "Build Directory:\n";
    std::cout << "  Relative: " << makeRelative(buildDir).string() << "\n";
    std::cout << "  Absolute: " << buildDir.string() << "\n";
    std::cout << "  Exists: " << (std::filesystem::exists(buildDir) ? "Yes" : "No") << "\n";

    auto depsDir = getDependenciesDir();
    std::cout << "Dependencies Directory:\n";
    std::cout << "  Relative: " << makeRelative(depsDir).string() << "\n";
    std::cout << "  Absolute: " << depsDir.string() << "\n";
    std::cout << "  Exists: " << (std::filesystem::exists(depsDir) ? "Yes" : "No") << "\n";

    auto templatesDir = getTemplatesDir();
    std::cout << "Templates Directory:\n";
    std::cout << "  Relative: " << makeRelative(templatesDir).string() << "\n";
    std::cout << "  Absolute: " << templatesDir.string() << "\n";
    std::cout << "  Exists: " << (std::filesystem::exists(templatesDir) ? "Yes" : "No") << "\n";

    auto logsDir = getLogsDir();
    std::cout << "Logs Directory:\n";
    std::cout << "  Relative: " << makeRelative(logsDir).string() << "\n";
    std::cout << "  Absolute: " << logsDir.string() << "\n";
    std::cout << "  Exists: " << (std::filesystem::exists(logsDir) ? "Yes" : "No") << "\n";

    auto configFile = getConfigFile();
    std::cout << "Config File:\n";
    std::cout << "  Relative: " << makeRelative(configFile).string() << "\n";
    std::cout << "  Absolute: " << configFile.string() << "\n";
    std::cout << "  Exists: " << (std::filesystem::exists(configFile) ? "Yes" : "No") << "\n";

    std::cout << "========================================\n";
    std::cout << "Project paths will be:\n";
    std::cout << "  Project: " << makeRelative(getProjectDir("<ProjectName>")).string() << "\n";
    std::cout << "  Output: " << makeRelative(getProjectOutputDir("<ProjectName>")).string() << "\n";
    std::cout << "  Build: " << makeRelative(getProjectBuildDir("<ProjectName>")).string() << "\n";
    std::cout << "========================================\n";
}

} // namespace GameEngine
