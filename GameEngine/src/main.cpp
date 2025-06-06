#include "engine.h"
#include "cli/cli_engine.h"
#include "cli/cli_argument_parser.h"
#include <spdlog/spdlog.h>
#include <iostream>

#ifdef __APPLE__
    #include <pthread.h>
#endif

int main(int argc, char* argv[]) {
    #ifdef __APPLE__
        // Set thread priority for macOS to prevent throttling
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    #endif
    
    // Parse command line arguments
    auto args = CLIArgumentParser::parse(argc, argv);
    
    if (args.help) {
        CLIArgumentParser::printHelp();
        return 0;
    }
    
    if (args.version) {
        CLIArgumentParser::printVersion();
        return 0;
    }
    
    // If running in CLI mode
    if (args.mode != CLIMode::INTERACTIVE) {
        // Create CLI engine
        CLIEngine cliEngine;
        
        if (!cliEngine.initialize(args.mode, argc, argv)) {
            if (args.jsonOutput) {
                std::cout << CLIResult::Failure("Failed to initialize engine").toJson().dump() << std::endl;
            } else {
                std::cerr << "Error: Failed to initialize engine" << std::endl;
            }
            return 3;
        }
        
        CLIResult result;
        
        // Open project if specified
        if (!args.projectPath.empty()) {
            result = cliEngine.openProject(args.projectPath);
            if (!result.success) {
                if (args.jsonOutput) {
                    std::cout << result.toJson().dump() << std::endl;
                } else {
                    std::cerr << "Error: " << result.error << std::endl;
                }
                return 4;
            }
        }
        
        // Execute command based on mode
        switch (args.mode) {
            case CLIMode::SINGLE_COMMAND:
                result = cliEngine.executeCommand(args.command);
                break;
                
            case CLIMode::BATCH:
                if (!args.scriptPath.empty()) {
                    result = cliEngine.executeBatch(args.scriptPath);
                } else {
                    result = cliEngine.executeBatch(args.batchCommands);
                }
                break;
                
            default:
                result = CLIResult::Failure("Invalid CLI mode");
                break;
        }
        
        // Output result
        if (args.jsonOutput) {
            std::cout << result.toJson().dump() << std::endl;
        } else {
            if (result.success) {
                std::cout << result.output << std::endl;
            } else {
                std::cerr << "Error: " << result.error << std::endl;
            }
        }
        
        return result.exitCode;
    }
    
    // Otherwise run in interactive mode (original behavior)
    spdlog::info("Game Engine starting in interactive mode...");
    
    // Create engine instance
    Engine engine;
    
    // Initialize engine (loads config automatically)
    if (!engine.initialize()) {
        spdlog::error("Failed to initialize engine");
        return -1;
    }
    
    // Run the engine
    engine.run();
    
    // Shutdown
    engine.shutdown();
    
    spdlog::info("Game Engine terminated");
    return 0;
}