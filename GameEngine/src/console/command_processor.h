#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <string>

class Console;

using CommandFunction = std::function<void(const std::vector<std::string>&)>;

struct CommandInfo {
    CommandFunction function;
    std::string help;
};

class CommandProcessor {
private:
    std::unordered_map<std::string, CommandInfo> commands;
    Console* console;

public:
    CommandProcessor() = default;
    ~CommandProcessor() = default;
    
    void initialize(Console* console);
    void registerCommand(const std::string& name, CommandFunction func, const std::string& help = "");
    void executeCommand(const std::string& input);
    std::vector<std::string> parseCommand(const std::string& input);
    void registerDefaultCommands();
    
    // Helper to get all command names for autocomplete
    std::vector<std::string> getCommandNames() const;
    std::string getCommandHelp(const std::string& name) const;
};