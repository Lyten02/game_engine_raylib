#pragma once
#include <string>
#include <vector>

enum class CLIMode {
    INTERACTIVE,    // Normal mode with window (current)
    BATCH,         // Execute script commands
    SINGLE_COMMAND // One command and exit
};

class CLIArgumentParser {
public:
    struct ParsedArgs {
        CLIMode mode = CLIMode::INTERACTIVE;
        std::string projectPath;
        std::string command;
        std::string scriptPath;
        std::vector<std::string> batchCommands;
        bool verbose = false;
        bool jsonOutput = false;
        bool help = false;
        bool version = false;
        bool quiet = false;  // Suppress non-critical logs
        bool headless = false;  // Run without graphics
        std::string logLevel = "";  // Override log level
    };

    static ParsedArgs parse(int argc, char* argv[]);
    static void printHelp();
    static void printVersion();

private:
    static std::string getUsageString();
};
