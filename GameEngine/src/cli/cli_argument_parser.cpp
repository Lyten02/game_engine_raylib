#include "cli_argument_parser.h"
#include <iostream>

CLIArgumentParser::ParsedArgs CLIArgumentParser::parse(int argc, char* argv[]) {
    ParsedArgs args;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            args.help = true;
        }
        else if (arg == "--version" || arg == "-v") {
            args.version = true;
        }
        else if (arg == "--json") {
            args.jsonOutput = true;
        }
        else if (arg == "--headless") {
            args.headless = true;
        }
        else if (arg == "--command" || arg == "-c") {
            if (i + 1 < argc) {
                args.command = argv[++i];
                args.mode = CLIMode::SINGLE_COMMAND;
            }
        }
        else if (arg == "--project" || arg == "-p") {
            if (i + 1 < argc) {
                args.projectPath = argv[++i];
            }
        }
        else if (arg == "--batch") {
            args.mode = CLIMode::BATCH;
            // Collect remaining arguments as commands
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                args.batchCommands.push_back(argv[++i]);
            }
        }
        else if (arg == "--script") {
            if (i + 1 < argc) {
                args.scriptPath = argv[++i];
                args.mode = CLIMode::BATCH;
            }
        }
        else if (arg == "--verbose") {
            args.verbose = true;
        }
        else if (arg == "--quiet" || arg == "-q") {
            args.quiet = true;
        }
        else if (arg == "--log-level") {
            if (i + 1 < argc) {
                args.logLevel = argv[++i];
            }
        }
    }
    
    return args;
}

void CLIArgumentParser::printHelp() {
    std::cout << getUsageString() << std::endl;
}

void CLIArgumentParser::printVersion() {
    std::cout << "GameEngine v0.1.0" << std::endl;
    std::cout << "A 2D game engine with CLI support" << std::endl;
}

std::string CLIArgumentParser::getUsageString() {
    return R"(GameEngine - 2D Game Engine with CLI Support

Usage: GameEngine [OPTIONS]

Options:
  -h, --help              Show this help message
  -v, --version           Show version information
  --json                  Output results as JSON
  --headless              Run without graphics window
  -c, --command CMD       Execute single command and exit
  -p, --project PATH      Open project before executing commands
  --batch CMD1 CMD2...    Execute multiple commands
  --script FILE           Execute commands from script file
  --verbose               Enable verbose output
  -q, --quiet             Suppress non-critical logs
  --log-level LEVEL       Set log level (trace/debug/info/warn/error/off)

Examples:
  GameEngine                                  Launch in interactive mode
  GameEngine --json --command "help"          List available commands as JSON
  GameEngine --headless -c "project.list"     List projects without GUI
  GameEngine --script tests/test.txt          Run test script
  GameEngine --batch "project.create test" "entity.create Player"
  
For more information, visit: https://github.com/yourgithub/gameengine)"
    ;
}