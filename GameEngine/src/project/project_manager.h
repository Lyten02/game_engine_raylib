#pragma once

#include <string>
#include <vector>
#include <memory>
#include "project.h"

namespace GameEngine {
class ProjectManager {
private:
    std::unique_ptr<Project> currentProject;
    std::string projectsRootPath = "projects/";
    std::vector<std::string> recentProjects;

public:
    bool createProject(const std::string& name, const std::string& template_name = "basic");
    bool openProject(const std::string& name);
    bool closeProject();
    std::vector<std::string> listProjects();
    Project* getCurrentProject();
    bool projectExists(const std::string& name);
};

} // namespace GameEngine
