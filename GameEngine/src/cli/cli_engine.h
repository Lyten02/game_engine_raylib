#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "cli_argument_parser.h"
#include "cli_result.h"

class Engine; // Forward declaration

// CLI Engine Wrapper
class CLIEngine {
public:
    CLIEngine();
    ~CLIEngine();

    // Initialize in different modes
    bool initialize(CLIMode mode, bool headless, int argc, char* argv[]);

    // Execute commands
    CLIResult executeCommand(const std::string& command);
    CLIResult executeBatch(const std::string& scriptPath);
    CLIResult executeBatch(const std::vector<std::string>& commands);

    // Project management
    CLIResult openProject(const std::string& projectPath);
    CLIResult closeProject();

    // Get state
    bool isHeadless() const { return m_headless; }
    bool isInitialized() const { return m_engine != nullptr; }

    // For testing
    void setTestMode(bool enabled) { m_testMode = enabled; }

private:
    CLIMode m_mode;
    std::unique_ptr<Engine> m_engine;
    bool m_headless = false;
    bool m_testMode = false;

    // Initialize subsystems in headless mode
    bool initializeHeadless();
    bool initializeGraphics();
};
