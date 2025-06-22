#include "build_config.h"
#include <sstream>

namespace GameEngine {
BuildConfig::BuildConfig(BuildType type) : buildType(type) {
    // Set default options based on build type
    switch (buildType) {
        case BuildType::Debug:
            compilerOptions.optimizationEnabled = false;
            compilerOptions.optimizationLevel = 0;
            compilerOptions.defines.push_back("DEBUG");
            compilerOptions.flags.push_back("-g");
            break;

        case BuildType::Release:
            compilerOptions.optimizationEnabled = true;
            compilerOptions.optimizationLevel = 2;
            compilerOptions.defines.push_back("NDEBUG");
            linkerOptions.stripSymbols = true;
            break;

        case BuildType::RelWithDebInfo:
            compilerOptions.optimizationEnabled = true;
            compilerOptions.optimizationLevel = 2;
            compilerOptions.defines.push_back("NDEBUG");
            compilerOptions.flags.push_back("-g");
            break;

        case BuildType::MinSizeRel:
            compilerOptions.optimizationEnabled = true;
            compilerOptions.optimizationLevel = -1; // -Os
            compilerOptions.defines.push_back("NDEBUG");
            linkerOptions.stripSymbols = true;
            break;
    }

    // Common flags
    compilerOptions.flags.push_back("-Wall");
    compilerOptions.flags.push_back("-Wextra");
}

void BuildConfig::setBuildType(BuildType type) {
    buildType = type;
}

std::string BuildConfig::getCMakeBuildType() const {
    switch (buildType) {
        case BuildType::Debug: return "Debug";
        case BuildType::Release: return "Release";
        case BuildType::RelWithDebInfo: return "RelWithDebInfo";
        case BuildType::MinSizeRel: return "MinSizeRel";
    }
    return "Release";
}

std::string BuildConfig::getCompilerFlags() const {
    std::stringstream ss;

    // Add optimization flag
    if (compilerOptions.optimizationEnabled) {
        if (compilerOptions.optimizationLevel == -1) {
            ss << "-Os ";
        } else {
            ss << "-O" << compilerOptions.optimizationLevel << " ";
        }
    } else {
        ss << "-O0 ";
    }

    // Add other flags
    for (const auto& flag : compilerOptions.flags) {
        ss << flag << " ";
    }

    // Add defines
    for (const auto& define : compilerOptions.defines) {
        ss << "-D" << define << " ";
    }

    return ss.str();
}

std::string BuildConfig::getLinkerFlags() const {
    std::stringstream ss;

    for (const auto& flag : linkerOptions.flags) {
        ss << flag << " ";
    }

    if (linkerOptions.stripSymbols) {
        ss << "-s ";
    }

    return ss.str();
}

nlohmann::json BuildConfig::toJson() const {
    return {
        {"buildType", getCMakeBuildType()},
        {"compiler", {
            {"standard", compilerOptions.standard},
            {"optimization", compilerOptions.optimizationLevel},
            {"flags", compilerOptions.flags},
            {"defines", compilerOptions.defines}
        }},
        {"linker", {
            {"flags", linkerOptions.flags},
            {"libraries", linkerOptions.libraries},
            {"libraryPaths", linkerOptions.libraryPaths},
            {"stripSymbols", linkerOptions.stripSymbols}
        }}
    };
}

void BuildConfig::fromJson(const nlohmann::json& j) {
    if (j.contains("buildType")) {
        std::string typeStr = j["buildType"];
        if (typeStr == "Debug") buildType = BuildType::Debug;
        else if (typeStr == "Release") buildType = BuildType::Release;
        else if (typeStr == "RelWithDebInfo") buildType = BuildType::RelWithDebInfo;
        else if (typeStr == "MinSizeRel") buildType = BuildType::MinSizeRel;
    }

    if (j.contains("compiler")) {
        const auto& compiler = j["compiler"];
        if (compiler.contains("standard")) compilerOptions.standard = compiler["standard"];
        if (compiler.contains("optimization")) compilerOptions.optimizationLevel = compiler["optimization"];
        if (compiler.contains("flags")) compilerOptions.flags = compiler["flags"].get<std::vector<std::string >> ();
        if (compiler.contains("defines")) compilerOptions.defines = compiler["defines"].get<std::vector<std::string >> ();
    }

    if (j.contains("linker")) {
        const auto& linker = j["linker"];
        if (linker.contains("flags")) linkerOptions.flags = linker["flags"].get<std::vector<std::string >> ();
        if (linker.contains("libraries")) linkerOptions.libraries = linker["libraries"].get<std::vector<std::string >> ();
        if (linker.contains("libraryPaths")) linkerOptions.libraryPaths = linker["libraryPaths"].get<std::vector<std::string >> ();
        if (linker.contains("stripSymbols")) linkerOptions.stripSymbols = linker["stripSymbols"];
    }
}

BuildConfig BuildConfig::getDefaultDebugConfig() {
    return BuildConfig(BuildType::Debug);
}

BuildConfig BuildConfig::getDefaultReleaseConfig() {
    return BuildConfig(BuildType::Release);
}

} // namespace GameEngine
