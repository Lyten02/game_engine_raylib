#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace GameEngine {

class BuildConfig {
public:
    enum class BuildType {
        Debug,
        Release,
        RelWithDebInfo,
        MinSizeRel
    };
    
    struct CompilerOptions {
        std::vector<std::string> flags;
        std::vector<std::string> defines;
        std::string standard = "c++20";
        bool optimizationEnabled = true;
        int optimizationLevel = 2; // -O2
    };
    
    struct LinkerOptions {
        std::vector<std::string> flags;
        std::vector<std::string> libraries;
        std::vector<std::string> libraryPaths;
        bool stripSymbols = false;
    };
    
private:
    BuildType buildType = BuildType::Release;
    CompilerOptions compilerOptions;
    LinkerOptions linkerOptions;
    std::string targetPlatform;
    
public:
    BuildConfig(BuildType type = BuildType::Release);
    
    void setBuildType(BuildType type);
    BuildType getBuildType() const { return buildType; }
    
    CompilerOptions& getCompilerOptions() { return compilerOptions; }
    LinkerOptions& getLinkerOptions() { return linkerOptions; }
    
    std::string getCMakeBuildType() const;
    std::string getCompilerFlags() const;
    std::string getLinkerFlags() const;
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
    
    static BuildConfig getDefaultDebugConfig();
    static BuildConfig getDefaultReleaseConfig();
};

} // namespace GameEngine