#pragma once

#include <string>
#include <filesystem>

namespace PathUtils {
    // Get the base path for templates relative to the executable or project root
    std::string getTemplatePath();
    
    // Resolve a template file path
    std::string resolveTemplatePath(const std::string& relativePath);
    
    // Get the project root directory (GameEngine folder)
    std::string getProjectRoot();
}