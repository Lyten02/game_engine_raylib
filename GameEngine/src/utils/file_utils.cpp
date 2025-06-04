#include "file_utils.h"
#include <filesystem>
#include <fstream>
#include <sstream>

bool fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

std::string getFileExtension(const std::string& path) {
    std::filesystem::path p(path);
    return p.extension().string();
}

namespace FileUtils {
    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
}