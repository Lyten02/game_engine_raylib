#pragma once

#include <string>
#include <vector>

std::vector<std::string> split(const std::string& str, char delimiter);
std::string trim(const std::string& str);
std::string toLowerCase(const std::string& str);
bool startsWith(const std::string& str, const std::string& prefix);

namespace StringUtils {
    std::string replace(const std::string& str, const std::string& from, const std::string& to);
}
