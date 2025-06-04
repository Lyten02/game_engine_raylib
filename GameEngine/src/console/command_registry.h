#pragma once

#include "command_processor.h"

// Macro for easy command registration
#define REGISTER_COMMAND(processor, name, func, help) \
    (processor)->registerCommand(name, func, help)

// Helper macros for common command patterns
#define REGISTER_SIMPLE_COMMAND(processor, name, code, help) \
    REGISTER_COMMAND(processor, name, [this](const std::vector<std::string>& args) { code; }, help)

// Validate argument count
inline bool validateArgCount(Console* console, const std::vector<std::string>& args, 
                            size_t expected, const std::string& usage) {
    if (args.size() != expected) {
        console->addLine("Invalid number of arguments. Expected " + 
                        std::to_string(expected) + ", got " + 
                        std::to_string(args.size()), RED);
        console->addLine("Usage: " + usage, GRAY);
        return false;
    }
    return true;
}

// Validate minimum argument count
inline bool validateMinArgCount(Console* console, const std::vector<std::string>& args, 
                               size_t minimum, const std::string& usage) {
    if (args.size() < minimum) {
        console->addLine("Too few arguments. Expected at least " + 
                        std::to_string(minimum) + ", got " + 
                        std::to_string(args.size()), RED);
        console->addLine("Usage: " + usage, GRAY);
        return false;
    }
    return true;
}