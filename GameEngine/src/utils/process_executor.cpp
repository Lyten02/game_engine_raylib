#include "process_executor.h"
#include <spdlog/spdlog.h>
#include <regex>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#endif

namespace GameEngine {

ProcessExecutor::ProcessResult ProcessExecutor::execute(
    const std::string& executable,
    const std::vector<std::string>& args,
    const std::string& workingDir,
    std::function<void(const std::string&)> outputCallback) {
    
    return executeStreaming(executable, args, workingDir, outputCallback, nullptr);
}

ProcessExecutor::ProcessResult ProcessExecutor::executeStreaming(
    const std::string& executable,
    const std::vector<std::string>& args,
    const std::string& workingDir,
    std::function<void(const std::string&)> outputCallback,
    std::function<void(const std::string&)> errorCallback) {

#ifdef _WIN32
    return executeWindows(executable, args, workingDir, outputCallback, errorCallback);
#else
    return executeUnix(executable, args, workingDir, outputCallback, errorCallback);
#endif
}

bool ProcessExecutor::isValidProjectName(const std::string& input) {
    // Only allow alphanumeric, underscore, and hyphen
    // No spaces, no special shell characters
    static std::regex validNameRegex("^[a-zA-Z0-9_-]+$");
    
    // Also check length limits
    if (input.empty() || input.length() > 255) {
        return false;
    }
    
    // Check against common shell metacharacters
    static const std::string dangerousChars = "&|;`$(){}[]<>*?!~#%^\\\"'";
    if (input.find_first_of(dangerousChars) != std::string::npos) {
        return false;
    }
    
    return std::regex_match(input, validNameRegex);
}

std::string ProcessExecutor::sanitizePath(const std::string& path, const std::string& basePath) {
    try {
        // Resolve to absolute paths
        std::filesystem::path base = std::filesystem::absolute(basePath);
        std::filesystem::path target = std::filesystem::absolute(path);
        
        // Canonicalize to resolve .. and symlinks
        if (std::filesystem::exists(base)) {
            base = std::filesystem::canonical(base);
        }
        
        if (std::filesystem::exists(target)) {
            target = std::filesystem::canonical(target);
        }
        
        // Check if target is within base
        std::string baseStr = base.string();
        std::string targetStr = target.string();
        
        if (targetStr.find(baseStr) != 0) {
            spdlog::warn("Path traversal attempt blocked: {} not within {}", targetStr, baseStr);
            return "";
        }
        
        return targetStr;
    } catch (const std::exception& e) {
        spdlog::error("Path sanitization failed: {}", e.what());
        return "";
    }
}

#ifndef _WIN32
// Unix/Linux/macOS implementation
ProcessExecutor::ProcessResult ProcessExecutor::executeUnix(
    const std::string& executable,
    const std::vector<std::string>& args,
    const std::string& workingDir,
    std::function<void(const std::string&)> outputCallback,
    std::function<void(const std::string&)> errorCallback) {
    
    ProcessResult result;
    result.exitCode = -1;
    result.success = false;
    
    // Create pipes for stdout and stderr
    int pipeOut[2], pipeErr[2];
    if (pipe(pipeOut) == -1 || pipe(pipeErr) == -1) {
        result.error = "Failed to create pipes";
        return result;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        close(pipeOut[0]); close(pipeOut[1]);
        close(pipeErr[0]); close(pipeErr[1]);
        result.error = "Failed to fork process";
        return result;
    }
    
    if (pid == 0) {
        // Child process
        // Close read ends
        close(pipeOut[0]);
        close(pipeErr[0]);
        
        // Redirect stdout and stderr
        dup2(pipeOut[1], STDOUT_FILENO);
        dup2(pipeErr[1], STDERR_FILENO);
        
        // Close write ends
        close(pipeOut[1]);
        close(pipeErr[1]);
        
        // Change working directory if specified
        if (!workingDir.empty()) {
            if (chdir(workingDir.c_str()) != 0) {
                std::cerr << "Failed to change directory to: " << workingDir << std::endl;
                _exit(1);
            }
        }
        
        // Prepare arguments for execvp
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        // Execute - this replaces the current process
        execvp(executable.c_str(), argv.data());
        
        // If we get here, exec failed
        std::cerr << "Failed to execute: " << executable << std::endl;
        _exit(1);
    }
    
    // Parent process
    // Close write ends
    close(pipeOut[1]);
    close(pipeErr[1]);
    
    // Make pipes non-blocking
    fcntl(pipeOut[0], F_SETFL, O_NONBLOCK);
    fcntl(pipeErr[0], F_SETFL, O_NONBLOCK);
    
    // Read output from pipes
    std::stringstream outputStream, errorStream;
    char buffer[4096];
    bool running = true;
    
    while (running) {
        // Check if child is still running
        int status;
        pid_t wpid = waitpid(pid, &status, WNOHANG);
        if (wpid == pid) {
            running = false;
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.exitCode = -WTERMSIG(status);
                result.error = "Process terminated by signal";
            }
        }
        
        // Read stdout
        ssize_t bytesRead = read(pipeOut[0], buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string chunk(buffer);
            outputStream << chunk;
            
            // Process line by line for callback
            if (outputCallback) {
                std::istringstream lines(chunk);
                std::string line;
                while (std::getline(lines, line)) {
                    if (!line.empty()) {
                        outputCallback(line);
                    }
                }
            }
        }
        
        // Read stderr
        bytesRead = read(pipeErr[0], buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string chunk(buffer);
            errorStream << chunk;
            
            if (errorCallback) {
                std::istringstream lines(chunk);
                std::string line;
                while (std::getline(lines, line)) {
                    if (!line.empty()) {
                        errorCallback(line);
                    }
                }
            }
        }
        
        // Small delay to prevent busy waiting
        if (running) {
            usleep(10000); // 10ms
        }
    }
    
    // Final read to get any remaining data
    while ((read(pipeOut[0], buffer, sizeof(buffer) - 1)) > 0 ||
           (read(pipeErr[0], buffer, sizeof(buffer) - 1)) > 0) {
        // Continue reading until pipes are empty
    }
    
    close(pipeOut[0]);
    close(pipeErr[0]);
    
    result.output = outputStream.str();
    result.error = errorStream.str();
    result.success = (result.exitCode == 0);
    
    return result;
}

#else
// Windows implementation
ProcessExecutor::ProcessResult ProcessExecutor::executeWindows(
    const std::string& executable,
    const std::vector<std::string>& args,
    const std::string& workingDir,
    std::function<void(const std::string&)> outputCallback,
    std::function<void(const std::string&)> errorCallback) {
    
    ProcessResult result;
    result.exitCode = -1;
    result.success = false;
    
    // Build command line
    std::string cmdLine = "\"" + executable + "\"";
    for (const auto& arg : args) {
        cmdLine += " \"" + arg + "\"";
    }
    
    // Set up security attributes
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    
    // Create pipes for stdout and stderr
    HANDLE hStdOutRead, hStdOutWrite;
    HANDLE hStdErrRead, hStdErrWrite;
    
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0) ||
        !CreatePipe(&hStdErrRead, &hStdErrWrite, &saAttr, 0)) {
        result.error = "Failed to create pipes";
        return result;
    }
    
    // Ensure read handles are not inherited
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0);
    
    // Set up process info
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hStdErrWrite;
    si.hStdOutput = hStdOutWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;
    
    ZeroMemory(&pi, sizeof(pi));
    
    // Create process
    BOOL success = CreateProcessA(
        NULL,
        const_cast<char*>(cmdLine.c_str()),
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        workingDir.empty() ? NULL : workingDir.c_str(),
        &si,
        &pi
    );
    
    if (!success) {
        CloseHandle(hStdOutRead);
        CloseHandle(hStdOutWrite);
        CloseHandle(hStdErrRead);
        CloseHandle(hStdErrWrite);
        result.error = "Failed to create process";
        return result;
    }
    
    // Close write ends
    CloseHandle(hStdOutWrite);
    CloseHandle(hStdErrWrite);
    
    // Read output
    std::stringstream outputStream, errorStream;
    CHAR buffer[4096];
    DWORD bytesRead;
    
    // Read from pipes
    while (true) {
        // Check if process is still running
        DWORD exitCode;
        if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
            result.exitCode = exitCode;
            break;
        }
        
        // Read stdout
        DWORD available;
        if (PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &available, NULL) && available > 0) {
            if (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string chunk(buffer);
                outputStream << chunk;
                
                if (outputCallback) {
                    std::istringstream lines(chunk);
                    std::string line;
                    while (std::getline(lines, line)) {
                        if (!line.empty()) {
                            outputCallback(line);
                        }
                    }
                }
            }
        }
        
        // Read stderr
        if (PeekNamedPipe(hStdErrRead, NULL, 0, NULL, &available, NULL) && available > 0) {
            if (ReadFile(hStdErrRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string chunk(buffer);
                errorStream << chunk;
                
                if (errorCallback) {
                    std::istringstream lines(chunk);
                    std::string line;
                    while (std::getline(lines, line)) {
                        if (!line.empty()) {
                            errorCallback(line);
                        }
                    }
                }
            }
        }
        
        Sleep(10); // 10ms delay
    }
    
    // Wait for process to finish
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    // Clean up
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdOutRead);
    CloseHandle(hStdErrRead);
    
    result.output = outputStream.str();
    result.error = errorStream.str();
    result.success = (result.exitCode == 0);
    
    return result;
}
#endif

} // namespace GameEngine