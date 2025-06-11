#pragma once

#include <string>
#include <memory>

namespace GameEngine {

class Project;

class BuildSystem {
private:
    std::string outputPath = "output/";
    std::string runtimeTemplatePath = "runtime/";
    std::string currentBuildTarget;

public:
    bool buildProject(Project* project, const std::string& buildConfig = "Release");
    bool generateGameCode(Project* project, const std::string& outputDir);
    bool compileProject(Project* project, const std::string& projectDir, const std::string& outputPath);
    bool packageAssets(Project* project, const std::string& outputDir);
    bool processScenes(Project* project, const std::string& outputDir);
    bool generateCMakeLists(Project* project, const std::string& outputDir);
    bool generateCMakeListsFast(Project* project, const std::string& outputDir);
    bool createBuildDirectory(const std::string& projectName);
    
private:
    bool copyRuntimeLibrary(const std::string& outputDir);
    std::string processTemplate(const std::string& templateContent, Project* project);
};

} // namespace GameEngine