#include "engine.h"
#include "cli/cli_engine.h"
#include "cli/cli_argument_parser.h"
#include "utils/log_limiter.h"
#include "utils/engine_paths.h"
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
    
    // Parse command line arguments FIRST
    auto args = CLIArgumentParser::parse(argc, argv);
    
    // Configure logging based on command line arguments BEFORE any initialization
    if (!args.logLevel.empty()) {
        // Set specific log level if provided
        if (args.logLevel == "trace") spdlog::set_level(spdlog::level::trace);
        else if (args.logLevel == "debug") spdlog::set_level(spdlog::level::debug);
        else if (args.logLevel == "info") spdlog::set_level(spdlog::level::info);
        else if (args.logLevel == "warn") spdlog::set_level(spdlog::level::warn);
        else if (args.logLevel == "error") spdlog::set_level(spdlog::level::err);
        else if (args.logLevel == "off") spdlog::set_level(spdlog::level::off);
    } else if (args.quiet) {
        // Quiet mode - only errors and warnings
        spdlog::set_level(spdlog::level::warn);
    } else if (args.jsonOutput) {
        // Suppress all logging for JSON output
        spdlog::set_level(spdlog::level::off);
    } else if (args.verbose) {
        // Verbose mode - show debug logs
        spdlog::set_level(spdlog::level::debug);
    }
    
    // Initialize engine paths AFTER logging is configured
    GameEngine::EnginePaths::initialize();
    
    // Configure log limiting for test mode
    if (args.mode == CLIMode::BATCH || args.mode == CLIMode::SINGLE_COMMAND) {
        // In batch/test mode, limit repetitive messages
        GameEngine::LogLimiter::configure(3, 60, true);  // Max 3 messages per key per minute
    }
    
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