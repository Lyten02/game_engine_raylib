#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace GameEngine {
/**
 * Secure process execution utility that prevents command injection
 * by using exec-style APIs instead of shell interpretation
 */
class ProcessExecutor {
public:
    struct ProcessResult {
        int exitCode;
        std::string output;
        std::string error;
        bool success;
    };

    /**
     * Execute a process with arguments as an array (no shell interpretation)
     * @param executable The program to execute
     * @param args Arguments to pass (as separate strings, not concatenated)
     * @param workingDir Optional working directory
     * @param outputCallback Optional callback for streaming output
     * @return ProcessResult with exit code and captured output
     */
    static ProcessResult execute(
        const std::string& executable,
        const std::vector<std::string>& args,
        const std::string& workingDir = "",
        std::function<void(const std::string&)> outputCallback = nullptr
    );

    /**
     * Execute a process asynchronously with streaming output
     * @param executable The program to execute
     * @param args Arguments to pass (as separate strings)
     * @param workingDir Optional working directory
     * @param outputCallback Callback for each line of output
     * @param errorCallback Callback for error output
     * @return ProcessResult
     */
    static ProcessResult executeStreaming(
        const std::string& executable,
        const std::vector<std::string>& args,
        const std::string& workingDir = "",
        std::function<void(const std::string&)> outputCallback = nullptr,
        std::function<void(const std::string&)> errorCallback = nullptr
    );

    /**
     * Validate that a string is safe for use as a filename/project name
     * @param input The string to validate
     * @return true if safe, false otherwise
     */
    static bool isValidProjectName(const std::string& input);

    /**
     * Sanitize a path to prevent directory traversal
     * @param path The path to sanitize
     * @param basePath The base directory that paths must be within
     * @return Sanitized path or empty string if invalid
     */
    static std::string sanitizePath(const std::string& path, const std::string& basePath);

private:
    // Platform-specific implementation
#ifdef _WIN32
    static ProcessResult executeWindows(
        const std::string& executable,
        const std::vector<std::string>& args,
        const std::string& workingDir,
        std::function<void(const std::string&)> outputCallback,
        std::function<void(const std::string&)> errorCallback
    );
#else
    static ProcessResult executeUnix(
        const std::string& executable,
        const std::vector<std::string>& args,
        const std::string& workingDir,
        std::function<void(const std::string&)> outputCallback,
        std::function<void(const std::string&)> errorCallback
    );
#endif
};

} // namespace GameEngine
