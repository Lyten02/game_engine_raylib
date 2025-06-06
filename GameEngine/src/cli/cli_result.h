#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct CLIResult {
    bool success;
    std::string output;
    std::string error;
    int exitCode;
    
    // For structured results
    nlohmann::json data;
    
    static CLIResult Success(const std::string& output, const nlohmann::json& data = {}) {
        return {true, output, "", 0, data};
    }
    
    static CLIResult Failure(const std::string& error, int exitCode = 1) {
        return {false, "", error, exitCode, {}};
    }
    
    // JSON serialization for Claude Coder
    nlohmann::json toJson() const {
        return {
            {"success", success},
            {"output", output},
            {"error", error},
            {"exit_code", exitCode},
            {"data", data}
        };
    }
};