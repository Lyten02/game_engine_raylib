#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>

class Console;

using CommandFunction = std::function<void(const std::vector<std::string>&)>;
using ParameterSuggestionProvider = std::function<std::vector<std::string>()>;

struct CommandParameter {
    std::string name;
    std::string description;
    bool required;
    ParameterSuggestionProvider suggestionProvider;  // Optional function to provide suggestions
};

struct CommandInfo {
    CommandFunction function;
    std::string help;
    std::string group;  // Command group for organization
    std::string syntax;  // Command syntax (e.g., "command <required> [optional]")
    std::vector<CommandParameter> parameters;  // Parameter descriptions
};

class CommandProcessor {
private:
    std::unordered_map<std::string, CommandInfo> commands;
    Console* console;
    int commandTimeoutSeconds = 10;
    bool timeoutEnabled = true;

public:
    CommandProcessor() = default;
    ~CommandProcessor() = default;
    
    void initialize(Console* console);
    void registerCommand(const std::string& name, CommandFunction func, 
                        const std::string& help = "", 
                        const std::string& group = "General",
                        const std::string& syntax = "",
                        const std::vector<CommandParameter>& params = {});
    void executeCommand(const std::string& input);
    std::vector<std::string> parseCommand(const std::string& input);
    void registerDefaultCommands();
    
    // Helper to get all command names for autocomplete
    std::vector<std::string> getCommandNames() const;
    std::string getCommandHelp(const std::string& name) const;
    CommandInfo getCommandInfo(const std::string& name) const;
    
    // Get commands organized by groups
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> getCommandsByGroup() const;
    
    // Get parameter suggestions for a command
    std::vector<std::string> getParameterSuggestions(const std::string& command, int paramIndex) const;
    
    // Timeout configuration
    void setCommandTimeout(int seconds) { commandTimeoutSeconds = seconds; }
    void setTimeoutEnabled(bool enabled) { timeoutEnabled = enabled; }
    int getCommandTimeout() const { return commandTimeoutSeconds; }
    bool isTimeoutEnabled() const { return timeoutEnabled; }
};