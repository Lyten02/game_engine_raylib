#pragma once

#include <string>

bool fileExists(const std::string& path);
std::string getFileExtension(const std::string& path);

namespace FileUtils {
    std::string readFile(const std::string& path);
}
