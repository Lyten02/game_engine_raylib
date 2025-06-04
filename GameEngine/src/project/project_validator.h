#pragma once

#include <string>
#include <vector>

namespace GameEngine {

class ProjectValidator {
public:
    struct ValidationResult {
        bool valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    static ValidationResult validateProject(const std::string& projectPath);
    static bool validateProjectStructure(const std::string& projectPath);
    static bool validateProjectFile(const std::string& projectFile);
    static bool validateSceneFile(const std::string& sceneFile);
    static bool validateResourceReferences(const std::string& projectPath);
};

} // namespace GameEngine