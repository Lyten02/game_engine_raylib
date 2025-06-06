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

void CommandProcessor::registerCommand(const std::string& name, CommandFunction func, 
                                     const std::string& help, const std::string& group,
                                     const std::string& syntax, const std::vector<CommandParameter>& params) {
    commands[name] = {func, help, group, syntax, params};
    spdlog::debug("CommandProcessor::registerCommand - Registered command: {} in group {}", name, group);
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
        if (!args.empty()) {
            // Show help for specific command
            std::string cmdName = args[0];
            auto it = commands.find(cmdName);
            if (it != commands.end()) {
                const auto& info = it->second;
                console->addLine("Command: " + cmdName, YELLOW);
                console->addLine("  " + info.help, WHITE);
                
                if (!info.syntax.empty()) {
                    console->addLine("  Syntax: " + info.syntax, SKYBLUE);
                }
                
                if (!info.parameters.empty()) {
                    console->addLine("  Parameters:", GREEN);
                    for (const auto& param : info.parameters) {
                        std::string paramLine = "    " + param.name;
                        if (param.required) {
                            paramLine += " (required)";
                        } else {
                            paramLine += " (optional)";
                        }
                        paramLine += " - " + param.description;
                        console->addLine(paramLine, LIGHTGRAY);
                    }
                }
                
                console->addLine("  Group: " + info.group, GRAY);
            } else {
                console->addLine("Unknown command: " + cmdName, RED);
            }
            return;
        }
        
        // Show all commands grouped
        auto groupedCommands = getCommandsByGroup();
        
        console->addLine("=== Available Commands ===", YELLOW);
        console->addLine("", WHITE);
        
        // Sort groups for consistent display
        std::vector<std::string> groups;
        for (const auto& [group, cmds] : groupedCommands) {
            groups.push_back(group);
        }
        std::sort(groups.begin(), groups.end());
        
        // Move "Package" group to the end if it exists
        auto packageIt = std::find(groups.begin(), groups.end(), "Package");
        if (packageIt != groups.end()) {
            groups.erase(packageIt);
            groups.push_back("Package");
        }
        
        // Display each group
        for (const auto& group : groups) {
            const auto& cmds = groupedCommands.at(group);
            
            // Special formatting for Package group
            if (group == "Package") {
                console->addLine("", WHITE);
                console->addLine("╔═══════════════════════════════════════╗", MAGENTA);
                console->addLine("║         PACKAGE COMMANDS              ║", MAGENTA);
                console->addLine("╚═══════════════════════════════════════╝", MAGENTA);
            } else {
                console->addLine("【 " + group + " 】", SKYBLUE);
            }
            
            // Sort commands within group
            auto sortedCmds = cmds;
            std::sort(sortedCmds.begin(), sortedCmds.end(),
                     [](const auto& a, const auto& b) { return a.first < b.first; });
            
            for (const auto& [cmd, help] : sortedCmds) {
                std::string helpLine = "  " + cmd;
                
                // Pad command name for alignment
                const size_t padLength = 25;
                if (cmd.length() < padLength) {
                    helpLine += std::string(padLength - cmd.length(), ' ');
                } else {
                    helpLine += " ";
                }
                
                helpLine += "- " + help;
                console->addLine(helpLine, group == "Package" ? VIOLET : WHITE);
            }
            console->addLine("", WHITE);
        }
        
        console->addLine("Type 'help <command>' for detailed information about a command.", GRAY);
    }, "Show this help message", "General", 
    "help [command]",
    {{"command", "Command name to get help for", false}});
    
    // Clear command
    registerCommand("clear", [this](const std::vector<std::string>& args) {
        console->clear();
    }, "Clear the console output", "General");
    
    // Quit command
    registerCommand("quit", [this](const std::vector<std::string>& args) {
        console->addLine("Shutting down...", YELLOW);
        // The actual quit will be handled by the engine
    }, "Quit the application", "General");
    
    // Alias for quit
    registerCommand("exit", [this](const std::vector<std::string>& args) {
        executeCommand("quit");
    }, "Quit the application", "General");
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

std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> CommandProcessor::getCommandsByGroup() const {
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> grouped;
    
    for (const auto& [name, info] : commands) {
        grouped[info.group].push_back({name, info.help});
    }
    
    return grouped;
}

CommandInfo CommandProcessor::getCommandInfo(const std::string& name) const {
    auto it = commands.find(name);
    if (it != commands.end()) {
        return it->second;
    }
    return CommandInfo{};
}

std::vector<std::string> CommandProcessor::getParameterSuggestions(const std::string& command, int paramIndex) const {
    auto it = commands.find(command);
    if (it != commands.end()) {
        const auto& params = it->second.parameters;
        spdlog::debug("Command '{}' has {} parameters, requesting index {}", command, params.size(), paramIndex);
        
        if (paramIndex >= 0 && paramIndex < static_cast<int>(params.size())) {
            if (params[paramIndex].suggestionProvider) {
                spdlog::debug("Parameter '{}' has suggestion provider", params[paramIndex].name);
                auto suggestions = params[paramIndex].suggestionProvider();
                spdlog::debug("Provider returned {} suggestions", suggestions.size());
                return suggestions;
            } else {
                spdlog::debug("Parameter '{}' has no suggestion provider", params[paramIndex].name);
            }
        } else {
            spdlog::debug("Parameter index {} out of range", paramIndex);
        }
    } else {
        spdlog::debug("Command '{}' not found", command);
    }
    return {};
}