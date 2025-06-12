#include "cpp_test_runner.h"
#include <iostream>
#include <cstring>

using namespace GameEngine::Testing;

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --help, -h           Show this help message\n";
    std::cout << "  --verbose, -v        Enable verbose output\n";
    std::cout << "  --parallel, -p       Run tests in parallel\n";
    std::cout << "  --workers <n>        Number of parallel workers (default: auto)\n";
    std::cout << "  --category <name>    Run only tests in specified category\n";
    std::cout << "                       (UNIT, INTEGRATION, BUILD, RESOURCE, THREADING, MEMORY, PERFORMANCE)\n";
    std::cout << "  --test <name>        Run only the specified test\n";
    std::cout << "  --skip-slow          Skip slow tests (BUILD, INTEGRATION, PERFORMANCE)\n";
    std::cout << "  --json <file>        Output JSON results to file (default: cpp_test_results.json)\n";
    std::cout << "  --log <file>         Output log to file (default: cpp_test_log_TIMESTAMP.log)\n";
    std::cout << "  --no-progress        Disable progress bar\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " --parallel --workers 4\n";
    std::cout << "  " << programName << " --category UNIT --verbose\n";
    std::cout << "  " << programName << " --test test_resource_manager_threading\n";
    std::cout << std::endl;
}

TestCategory parseCategory(const std::string& name) {
    if (name == "UNIT") return TestCategory::UNIT;
    if (name == "INTEGRATION") return TestCategory::INTEGRATION;
    if (name == "BUILD") return TestCategory::BUILD;
    if (name == "RESOURCE") return TestCategory::RESOURCE;
    if (name == "THREADING") return TestCategory::THREADING;
    if (name == "MEMORY") return TestCategory::MEMORY;
    if (name == "PERFORMANCE") return TestCategory::PERFORMANCE;
    
    std::cerr << "Unknown category: " << name << std::endl;
    std::cerr << "Valid categories: UNIT, INTEGRATION, BUILD, RESOURCE, THREADING, MEMORY, PERFORMANCE" << std::endl;
    exit(1);
}

int main(int argc, char* argv[]) {
    TestRunnerConfig config;
    std::string categoryName;
    std::string testName;
    bool showHelp = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            showHelp = true;
        } else if (arg == "--verbose" || arg == "-v") {
            config.verbose = true;
        } else if (arg == "--parallel" || arg == "-p") {
            config.parallel = true;
        } else if (arg == "--workers" && i + 1 < argc) {
            config.maxWorkers = std::stoi(argv[++i]);
        } else if (arg == "--category" && i + 1 < argc) {
            categoryName = argv[++i];
        } else if (arg == "--test" && i + 1 < argc) {
            testName = argv[++i];
        } else if (arg == "--skip-slow") {
            config.skipSlowTests = true;
        } else if (arg == "--json" && i + 1 < argc) {
            config.jsonOutputFile = argv[++i];
        } else if (arg == "--log" && i + 1 < argc) {
            config.logFile = argv[++i];
        } else if (arg == "--no-progress") {
            config.showProgress = false;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            showHelp = true;
        }
    }
    
    if (showHelp) {
        printUsage(argv[0]);
        return showHelp ? 0 : 1;
    }
    
    // Create test runner
    CppTestRunner runner(config);
    
    // Register all tests
    runner.registerAllDefaultTests();
    
    // Run tests based on options
    if (!testName.empty()) {
        runner.runTest(testName);
    } else if (!categoryName.empty()) {
        TestCategory category = parseCategory(categoryName);
        runner.runCategory(category);
    } else {
        runner.runAll();
    }
    
    // Return exit code based on test results
    return runner.getFailedCount() > 0 ? 1 : 0;
}