#include "command_processor.h"
#include "console.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <algorithm>

void CommandProcessor::initialize(Console* con) {
    console = con;
    registerDefaultCommands();
    spdlog::info("CommandProcessor::initialize - Command processor initialized");
}

void CommandProcessor::registerCommand(const std::string& name, CommandFunction func, const std::string& help) {
    commands[name] = {func, help};
    spdlog::debug("CommandProcessor::registerCommand - Registered command: {}", name);
}

std::vector<std::string> CommandProcessor::parseCommand(const std::string& input) {
    std::vector<std::string> tokens;
    std::stringstream ss(input);
    std::string token;
    bool inQuotes = false;
    std::string currentToken;
    
    for (char c : input) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ' ' && !inQuotes) {
            if (!currentToken.empty()) {
                tokens.push_back(currentToken);
                currentToken.clear();
            }
        } else {
            currentToken += c;
        }
    }
    
    if (!currentToken.empty()) {
        tokens.push_back(currentToken);
    }
    
    return tokens;
}

void CommandProcessor::executeCommand(const std::string& input) {
    if (!console) {
        spdlog::error("CommandProcessor::executeCommand - Console not initialized");
        return;
    }
    
    auto tokens = parseCommand(input);
    if (tokens.empty()) return;
    
    std::string commandName = tokens[0];
    std::transform(commandName.begin(), commandName.end(), commandName.begin(), ::tolower);
    
    auto it = commands.find(commandName);
    if (it != commands.end()) {
        try {
            std::vector<std::string> args(tokens.begin() + 1, tokens.end());
            it->second.function(args);
        } catch (const std::exception& e) {
            console->addLine("Error: " + std::string(e.what()), RED);
        }
    } else {
        console->addLine("Unknown command: " + commandName, RED);
        console->addLine("Type 'help' for a list of commands", GRAY);
    }
}

void CommandProcessor::registerDefaultCommands() {
    // Help command
    registerCommand("help", [this](const std::vector<std::string>& args) {
        console->addLine("Available commands:", YELLOW);
        for (const auto& [name, info] : commands) {
            std::string helpLine = "  " + name;
            if (!info.help.empty()) {
                helpLine += " - " + info.help;
            }
            console->addLine(helpLine, WHITE);
        }
    }, "Show this help message");
    
    // Clear command
    registerCommand("clear", [this](const std::vector<std::string>& args) {
        console->clear();
    }, "Clear the console output");
    
    // Quit command
    registerCommand("quit", [this](const std::vector<std::string>& args) {
        console->addLine("Shutting down...", YELLOW);
        // The actual quit will be handled by the engine
    }, "Quit the application");
    
    // Alias for quit
    registerCommand("exit", [this](const std::vector<std::string>& args) {
        executeCommand("quit");
    }, "Quit the application");
}

std::vector<std::string> CommandProcessor::getCommandNames() const {
    std::vector<std::string> names;
    for (const auto& [name, info] : commands) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::string CommandProcessor::getCommandHelp(const std::string& name) const {
    auto it = commands.find(name);
    if (it != commands.end()) {
        return it->second.help;
    }
    return "";
}