#include "file_utils.h"
#include <filesystem>

bool fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

std::string getFileExtension(const std::string& path) {
    std::filesystem::path p(path);
    return p.extension().string();
}