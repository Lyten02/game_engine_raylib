#include "cli_engine.h"
#include "../engine.h"
#include "../console/command_processor.h"
#include "../console/console.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>

CLIEngine::CLIEngine() = default;
CLIEngine::~CLIEngine() = default;

bool CLIEngine::initialize(CLIMode mode, bool headless, int argc, char* argv[]) {
    m_mode = mode;
    m_headless = headless;
    
    try {
        if (mode == CLIMode::BATCH || mode == CLIMode::SINGLE_COMMAND || headless) {
            return initializeHeadless();
        } else {
            return initializeGraphics();
        }
    } catch (const std::exception& e) {
        spdlog::error("CLI initialization failed: " + std::string(e.what()));
        return false;
    }
}

bool CLIEngine::initializeHeadless() {
    // Initialize engine without graphics
    m_engine = std::make_unique<Engine>();
    
    // Enable headless mode
    m_engine->setHeadlessMode(true);
    
    if (!m_engine->initialize()) {
        return false;
    }
    
    return true;
}

bool CLIEngine::initializeGraphics() {
    // Initialize engine with graphics
    m_engine = std::make_unique<Engine>();
    
    if (!m_engine->initialize()) {
        return false;
    }
    
    return true;
}

CLIResult CLIEngine::executeCommand(const std::string& command) {
    if (!m_engine) {
        return CLIResult::Failure("Engine not initialized");
    }
    
    try {
        auto* commandProcessor = m_engine->getCommandProcessor();
        if (!commandProcessor) {
            return CLIResult::Failure("Command processor not available");
        }
        
        // Get console
        auto* console = m_engine->getConsole();
        if (!console) {
            return CLIResult::Failure("Console not available");
        }
        
        // Clear any previous command data
        console->clearCommandData();
        
        // Enable capture mode
        console->enableCapture();
        
        // Execute command
        commandProcessor->executeCommand(command);
        
        // Get captured output and data
        std::string output = console->disableCapture();
        nlohmann::json data = console->getCommandData();
        
        // Check for errors in output
        bool hasError = output.find("Error:") != std::string::npos || 
                       output.find("Unknown command") != std::string::npos ||
                       output.find("Failed") != std::string::npos;
        
        if (hasError) {
            return CLIResult::Failure(output);
        } else {
            return CLIResult::Success(output, data);
        }
        
    } catch (const std::exception& e) {
        return CLIResult::Failure(std::string("Command execution failed: ") + e.what());
    }
}

CLIResult CLIEngine::executeBatch(const std::string& scriptPath) {
    // Read commands from file
    std::ifstream file(scriptPath);
    if (!file.is_open()) {
        return CLIResult::Failure("Failed to open script file: " + scriptPath);
    }
    
    std::vector<std::string> commands;
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (!line.empty() && line[0] != '#') {
            commands.push_back(line);
        }
    }
    file.close();
    
    return executeBatch(commands);
}

CLIResult CLIEngine::executeBatch(const std::vector<std::string>& commands) {
    nlohmann::json results = nlohmann::json::array();
    int successCount = 0;
    int failureCount = 0;
    
    for (const auto& command : commands) {
        CLIResult result = executeCommand(command);
        
        nlohmann::json cmdResult = {
            {"command", command},
            {"success", result.success},
            {"output", result.output},
            {"error", result.error}
        };
        results.push_back(cmdResult);
        
        if (result.success) {
            successCount++;
        } else {
            failureCount++;
            // If stop on error is enabled, break here
            if (!m_testMode) {
                break;
            }
        }
    }
    
    nlohmann::json batchData = {
        {"total_commands", commands.size()},
        {"successful_commands", successCount},
        {"failed_commands", failureCount},
        {"results", results}
    };
    
    if (failureCount == 0) {
        return CLIResult::Success("All commands executed successfully", batchData);
    } else {
        return CLIResult::Failure("Some commands failed", 1);
    }
}

CLIResult CLIEngine::openProject(const std::string& projectPath) {
    return executeCommand("project.open " + projectPath);
}

CLIResult CLIEngine::closeProject() {
    return executeCommand("project.close");
}