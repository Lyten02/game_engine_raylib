#include "cpp_test_runner.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <future>
#include <thread>
#include <cstdlib>
#include <chrono>
#include <sys/stat.h>

// Simple JSON serialization (to avoid dependency on nlohmann/json)
namespace json_utils {
    std::string escape_json(const std::string& s) {
        std::string result;
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }
}

namespace GameEngine::Testing {

// TestResult implementations
std::string TestResult::statusString() const {
    switch (status) {
        case TestStatus::PENDING: return "PENDING";
        case TestStatus::RUNNING: return "RUNNING";
        case TestStatus::PASSED: return "PASSED";
        case TestStatus::FAILED: return "FAILED";
        case TestStatus::TIMEOUT: return "TIMEOUT";
        case TestStatus::COMPILATION_FAILED: return "COMPILATION_FAILED";
        case TestStatus::SKIPPED: return "SKIPPED";
        default: return "UNKNOWN";
    }
}

std::string TestResult::toJson() const {
    std::stringstream json;
    json << "{\n";
    json << "  \"test_name\": \"" << json_utils::escape_json(testName) << "\",\n";
    json << "  \"test_file\": \"" << json_utils::escape_json(testFile) << "\",\n";
    json << "  \"status\": \"" << statusString() << "\",\n";
    json << "  \"elapsed_seconds\": " << std::fixed << std::setprecision(3) << elapsedSeconds << ",\n";
    json << "  \"return_code\": " << returnCode << ",\n";
    json << "  \"worker_id\": " << workerId << ",\n";
    json << "  \"timestamp\": " << std::chrono::system_clock::to_time_t(timestamp);
    
    if (!output.empty()) {
        json << ",\n  \"output\": \"" << json_utils::escape_json(output) << "\"";
    }
    if (!error.empty()) {
        json << ",\n  \"error\": \"" << json_utils::escape_json(error) << "\"";
    }
    
    json << "\n}";
    return json.str();
}

// CppTestRunner implementation
CppTestRunner::CppTestRunner(const TestRunnerConfig& cfg) : config(cfg) {
    startTime = std::chrono::system_clock::now();
    
    // Setup default include and library paths if not provided
    if (config.includePaths.empty()) {
        config.includePaths = {
            "../src",
            "../.deps_cache/_deps/raylib-src/src",
            "../.deps_cache/_deps/spdlog-src/include",
            "../.deps_cache/_deps/entt-src/src",
            "../.deps_cache/_deps/glm-src",
            "../.deps_cache/_deps/json-src/include"
        };
    }
    
    if (config.libraryPaths.empty()) {
        config.libraryPaths = {
            "../build",
            "../.deps_cache/_deps/raylib-build/raylib",
            "../.deps_cache/_deps/spdlog-build"
        };
    }
    
    if (config.libraries.empty()) {
        config.libraries = {"raylib", "spdlog"};
    }
    
    #ifdef __APPLE__
    if (config.frameworks.empty()) {
        config.frameworks = {"OpenGL", "Cocoa", "IOKit", "CoreVideo", "CoreFoundation"};
    }
    #endif
    
    // Setup log file
    if (!config.logFile.empty()) {
        logFile = std::make_unique<std::ofstream>(config.logFile);
        if (logFile->is_open()) {
            *logFile << "GameEngine C++ Test Suite Execution Log\n";
            *logFile << std::string(80, '=') << "\n";
            *logFile << "Start Time: " << getCurrentTimestamp() << "\n";
            *logFile << "Compiler: " << config.compiler << "\n";
            *logFile << "C++ Standard: " << config.cppStandard << "\n";
            *logFile << "Parallel Mode: " << (config.parallel ? "Yes" : "No") << "\n";
            if (config.parallel) {
                *logFile << "Max Workers: " << (config.maxWorkers == 0 ? "Auto" : std::to_string(config.maxWorkers)) << "\n";
            }
            *logFile << "Verbose Mode: " << (config.verbose ? "Yes" : "No") << "\n";
            *logFile << std::string(80, '=') << "\n\n";
        }
    }
}

CppTestRunner::~CppTestRunner() {
    if (logFile && logFile->is_open()) {
        logFile->close();
    }
}

void CppTestRunner::registerTest(const TestDefinition& test) {
    tests.push_back(test);
}

void CppTestRunner::registerAllDefaultTests() {
    // Resource Manager tests
    registerTest({"test_resource_manager_safety", "test_resource_manager_safety.cpp", TestCategory::RESOURCE});
    registerTest({"test_resource_manager_threading", "test_resource_manager_threading.cpp", TestCategory::THREADING});
    registerTest({"test_resource_manager_headless", "test_resource_manager_headless.cpp", TestCategory::RESOURCE});
    registerTest({"test_resource_manager_memory", "test_resource_manager_memory.cpp", TestCategory::MEMORY, 120});
    registerTest({"test_resource_manager_exception_safety", "test_resource_manager_exception_safety.cpp", TestCategory::RESOURCE});
    registerTest({"test_resource_manager_simple", "test_resource_manager_simple.cpp", TestCategory::UNIT});
    
    // Threading tests
    registerTest({"test_async_build_threading", "test_async_build_threading.cpp", TestCategory::THREADING});
    registerTest({"test_default_texture_manager", "test_default_texture_manager.cpp", TestCategory::THREADING});
    
    // Engine tests
    TestDefinition engineTest("test_engine_init", "test_engine_init.cpp", TestCategory::INTEGRATION);
    engineTest.additionalSources = {
        "../src/engine.cpp",
        "../src/engine/engine_core.cpp",
        "../src/engine/systems_manager.cpp",
        "../src/engine/command_registry.cpp",
        "../src/engine/command_registry_build.cpp",
        "../src/engine/command_registry_project.cpp",
        "../src/engine/command_registry_engine.cpp",
        "../src/engine/command_registry_entity.cpp",
        "../src/engine/command_registry_scene.cpp",
        "../src/systems/render_system.cpp",
        "../src/scene/scene.cpp",
        "../src/resources/resource_manager.cpp",
        "../src/console/console.cpp",
        "../src/console/command_processor.cpp",
        "../src/utils/file_utils.cpp",
        "../src/utils/string_utils.cpp",
        "../src/utils/config.cpp",
        "../src/utils/path_utils.cpp",
        "../src/scripting/script_manager.cpp",
        "../src/scripting/lua_bindings.cpp",
        "../src/project/project.cpp",
        "../src/project/project_manager.cpp",
        "../src/project/project_validator.cpp",
        "../src/serialization/scene_serializer.cpp",
        "../src/serialization/component_registry.cpp",
        "../src/build/build_system.cpp",
        "../src/build/build_config.cpp",
        "../src/build/async_build_system.cpp",
        "../src/engine/play_mode.cpp"
    };
    engineTest.timeoutSeconds = 60;
    registerTest(engineTest);
    
    // Build system tests
    TestDefinition buildTest("test_build_system_basic", "test_build_system_basic.cpp", TestCategory::BUILD);
    buildTest.additionalSources = {
        "../src/build/build_system.cpp",
        "../src/build/async_build_system.cpp",
        "../src/build/build_config.cpp",
        "../src/project/project.cpp",
        "../src/utils/file_utils.cpp",
        "../src/utils/string_utils.cpp",
        "../src/utils/path_utils.cpp"
    };
    registerTest(buildTest);
    
    // More unit tests
    registerTest({"test_config_depth", "test_config_depth.cpp", TestCategory::UNIT});
    registerTest({"test_log_limiter_generic_keys", "test_log_limiter_generic_keys.cpp", TestCategory::UNIT});
    registerTest({"test_script_manager_null_safety", "test_script_manager_null_safety.cpp", TestCategory::UNIT});
}

void CppTestRunner::runAll() {
    logMessage("Starting test execution of " + std::to_string(tests.size()) + " tests");
    
    if (config.parallel && tests.size() > 1) {
        runTestsParallel();
    } else {
        runTestsSequential();
    }
    
    // Calculate total elapsed time
    auto endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = endTime - startTime;
    totalElapsedTime = diff.count();
    
    // Save results
    if (!config.jsonOutputFile.empty()) {
        saveJsonResults();
    }
    
    if (logFile && logFile->is_open()) {
        saveLogFile();
    }
    
    // Print summary
    if (config.showProgress) {
        printSummary();
    }
}

void CppTestRunner::runCategory(TestCategory category) {
    std::vector<TestDefinition> categoryTests;
    for (const auto& test : tests) {
        if (test.category == category) {
            categoryTests.push_back(test);
        }
    }
    
    if (categoryTests.empty()) {
        std::cout << "No tests found in category: " << getCategoryName(category) << std::endl;
        return;
    }
    
    // Temporarily replace tests
    auto originalTests = std::move(tests);
    tests = std::move(categoryTests);
    
    runAll();
    
    // Restore original tests
    tests = std::move(originalTests);
}

void CppTestRunner::runTest(const std::string& testName) {
    for (const auto& test : tests) {
        if (test.name == testName) {
            currentTestIndex = 1;
            auto result = compileAndRunTest(test, 0);
            
            {
                std::lock_guard<std::mutex> lock(resultsMutex);
                results.push_back(result);
            }
            
            if (result.isSuccess()) {
                passedCount++;
            } else {
                failedCount++;
            }
            
            // Calculate total elapsed time
            auto endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> diff = endTime - startTime;
            totalElapsedTime = diff.count();
            
            updateProgress(result);
            
            if (!config.jsonOutputFile.empty()) {
                saveJsonResults();
            }
            
            if (config.showProgress) {
                printSummary();
            }
            
            return;
        }
    }
    
    std::cout << "Test not found: " << testName << std::endl;
}

void CppTestRunner::runTestsSequential() {
    int testIndex = 0;
    for (const auto& test : tests) {
        if (!shouldRunTest(test)) {
            testIndex++;
            continue;
        }
        
        currentTestIndex = testIndex + 1;
        auto result = compileAndRunTest(test, 0);
        
        {
            std::lock_guard<std::mutex> lock(resultsMutex);
            results.push_back(result);
        }
        
        if (result.isSuccess()) {
            passedCount++;
        } else {
            failedCount++;
        }
        
        updateProgress(result);
        testIndex++;
    }
}

void CppTestRunner::runTestsParallel() {
    int workerCount = config.maxWorkers;
    if (workerCount == 0) {
        workerCount = getOptimalWorkerCount();
    }
    
    logMessage("Running tests in parallel with " + std::to_string(workerCount) + " workers");
    
    // Create thread pool
    std::vector<std::future<void>> futures;
    std::atomic<int> testIndex{0};
    
    auto workerFunc = [this, &testIndex](int workerId) {
        while (true) {
            int idx = testIndex.fetch_add(1);
            if (idx >= static_cast<int>(tests.size())) {
                break;
            }
            
            const auto& test = tests[idx];
            if (!shouldRunTest(test)) {
                continue;
            }
            
            currentTestIndex = idx + 1;
            auto result = compileAndRunTest(test, workerId);
            
            {
                std::lock_guard<std::mutex> lock(resultsMutex);
                results.push_back(result);
            }
            
            if (result.isSuccess()) {
                passedCount++;
            } else {
                failedCount++;
            }
            
            updateProgress(result);
        }
    };
    
    // Launch workers
    for (int i = 0; i < workerCount; ++i) {
        futures.push_back(std::async(std::launch::async, workerFunc, i + 1));
    }
    
    // Wait for all workers to complete
    for (auto& future : futures) {
        future.wait();
    }
}

TestResult CppTestRunner::compileAndRunTest(const TestDefinition& test, int workerId) {
    TestResult result;
    result.testName = test.name;
    result.testFile = test.sourceFile;
    result.category = test.category;
    result.workerId = workerId;
    result.timestamp = std::chrono::system_clock::now();
    
    logTestStart(test, workerId);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Compile phase
    result = compileTest(test);
    if (result.status == TestStatus::COMPILATION_FAILED) {
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = endTime - startTime;
        result.elapsedSeconds = diff.count();
        logTestResult(result);
        return result;
    }
    
    // Run phase
    std::string executablePath = "./" + test.name;
    result = runCompiledTest(test, executablePath);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = endTime - startTime;
    result.elapsedSeconds = diff.count();
    
    // Clean up executable
    std::remove(executablePath.c_str());
    
    logTestResult(result);
    return result;
}

TestResult CppTestRunner::compileTest(const TestDefinition& test) {
    TestResult result;
    result.testName = test.name;
    result.testFile = test.sourceFile;
    result.category = test.category;
    result.status = TestStatus::RUNNING;
    
    std::string compileCommand = buildCompileCommand(test);
    
    if (config.verbose) {
        std::cout << "  Compiling: " << compileCommand << std::endl;
    }
    
    // Execute compilation
    FILE* pipe = popen((compileCommand + " 2>&1").c_str(), "r");
    if (!pipe) {
        result.status = TestStatus::COMPILATION_FAILED;
        result.error = "Failed to execute compiler";
        return result;
    }
    
    std::stringstream output;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output << buffer;
    }
    
    int returnCode = pclose(pipe);
    result.returnCode = WEXITSTATUS(returnCode);
    
    if (returnCode == 0) {
        result.status = TestStatus::PASSED; // Will be updated when running
        result.output = output.str();
    } else {
        result.status = TestStatus::COMPILATION_FAILED;
        result.error = output.str();
    }
    
    return result;
}

TestResult CppTestRunner::runCompiledTest(const TestDefinition& test, const std::string& executablePath) {
    TestResult result;
    result.testName = test.name;
    result.testFile = test.sourceFile;
    result.category = test.category;
    result.status = TestStatus::RUNNING;
    
    // Build run command with timeout
    std::string runCommand;
    #ifdef __APPLE__
        // macOS: use gtimeout if available, otherwise no timeout
        if (system("which gtimeout > /dev/null 2>&1") == 0) {
            runCommand = "gtimeout " + std::to_string(test.timeoutSeconds) + " " + executablePath;
        } else {
            runCommand = executablePath;
        }
    #else
        // Linux: use timeout command
        runCommand = "timeout " + std::to_string(test.timeoutSeconds) + " " + executablePath;
    #endif
    
    // Execute test
    FILE* pipe = popen((runCommand + " 2>&1").c_str(), "r");
    if (!pipe) {
        result.status = TestStatus::FAILED;
        result.error = "Failed to execute test";
        return result;
    }
    
    std::stringstream output;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output << buffer;
    }
    
    int returnCode = pclose(pipe);
    result.returnCode = WEXITSTATUS(returnCode);
    result.output = output.str();
    
    // Check for timeout (124 is the standard timeout exit code)
    if (result.returnCode == 124) {
        result.status = TestStatus::TIMEOUT;
        result.error = "Test timed out after " + std::to_string(test.timeoutSeconds) + " seconds";
    } else if (result.returnCode == 0) {
        result.status = TestStatus::PASSED;
    } else {
        result.status = TestStatus::FAILED;
        result.error = "Test failed with return code " + std::to_string(result.returnCode);
    }
    
    return result;
}

std::string CppTestRunner::buildCompileCommand(const TestDefinition& test) const {
    std::stringstream cmd;
    cmd << config.compiler << " " << config.cppStandard << " ";
    
    // Add source files
    cmd << test.sourceFile << " ";
    for (const auto& src : test.additionalSources) {
        cmd << src << " ";
    }
    
    // Add include paths
    for (const auto& flag : getIncludeFlags()) {
        cmd << flag << " ";
    }
    
    // Add library paths and libraries
    for (const auto& flag : getLibraryFlags()) {
        cmd << flag << " ";
    }
    
    // Add additional flags
    for (const auto& flag : test.additionalFlags) {
        cmd << flag << " ";
    }
    
    // Add pthread
    cmd << "-pthread ";
    
    // Output executable
    cmd << "-o " << test.name;
    
    // Suppress warnings in non-verbose mode
    if (!config.verbose) {
        cmd << " 2>/dev/null";
    }
    
    return cmd.str();
}

std::vector<std::string> CppTestRunner::getIncludeFlags() const {
    std::vector<std::string> flags;
    for (const auto& path : config.includePaths) {
        flags.push_back("-I" + path);
    }
    
    // Add Lua includes
    FILE* pipe = popen("pkg-config --cflags lua 2>/dev/null", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string luaIncludes(buffer);
            // Remove trailing newline
            luaIncludes.erase(luaIncludes.find_last_not_of(" \n\r\t") + 1);
            if (!luaIncludes.empty()) {
                flags.push_back(luaIncludes);
            }
        }
        pclose(pipe);
    }
    
    return flags;
}

std::vector<std::string> CppTestRunner::getLibraryFlags() const {
    std::vector<std::string> flags;
    
    // Library paths
    for (const auto& path : config.libraryPaths) {
        flags.push_back("-L" + path);
    }
    
    // Libraries
    for (const auto& lib : config.libraries) {
        flags.push_back("-l" + lib);
    }
    
    // Add Lua libraries
    FILE* pipe = popen("pkg-config --libs lua 2>/dev/null || echo -llua", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::string luaLibs(buffer);
            // Remove trailing newline
            luaLibs.erase(luaLibs.find_last_not_of(" \n\r\t") + 1);
            flags.push_back(luaLibs);
        }
        pclose(pipe);
    }
    
    // Frameworks (macOS)
    for (const auto& framework : config.frameworks) {
        flags.push_back("-framework");
        flags.push_back(framework);
    }
    
    return flags;
}

void CppTestRunner::updateProgress(const TestResult& result) {
    if (config.showProgress && progressCallback) {
        progressCallback(currentTestIndex.load(), static_cast<int>(tests.size()), result);
    } else if (config.showProgress) {
        printProgressBar(currentTestIndex.load(), static_cast<int>(tests.size()), result);
    }
}

void CppTestRunner::printProgressBar(int current, int total, const TestResult& result) {
    if (total == 0) return;
    
    // Calculate progress
    float percent = (static_cast<float>(current) / total) * 100.0f;
    int filled = static_cast<int>(percent / 4); // 25 chars for 100%
    
    // Build progress bar
    std::string bar;
    for (int i = 0; i < filled; ++i) bar += "█";
    for (int i = filled; i < 25; ++i) bar += "░";
    
    // Get status icon
    std::string icon = getStatusIcon(result.status);
    
    // Format test name (truncate if too long)
    std::string testName = result.testName;
    const size_t maxNameLen = 30;
    if (testName.length() > maxNameLen) {
        testName = testName.substr(0, maxNameLen - 3) + "...";
    }
    
    // Build output line
    std::stringstream line;
    line << "\r" << icon << " [" << bar << "] " 
         << std::fixed << std::setprecision(1) << std::setw(5) << percent << "% "
         << "(" << current << "/" << total << ") "
         << std::setw(maxNameLen) << std::left << testName
         << " (" << formatDuration(result.elapsedSeconds) << ")";
    
    // Add worker ID in parallel mode
    if (config.parallel && result.workerId > 0) {
        line << " [W" << result.workerId << "]";
    }
    
    // Print with padding to clear previous line
    std::cout << line.str() << std::string(10, ' ') << std::flush;
    
    // New line if test completed
    if (result.status != TestStatus::RUNNING) {
        std::cout << std::endl;
    }
}

void CppTestRunner::printSummary() const {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "TEST SUMMARY" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    int total = passedCount + failedCount;
    if (total == 0) {
        std::cout << "No tests were run!" << std::endl;
        return;
    }
    
    std::cout << "Total tests: " << total << std::endl;
    std::cout << "✅ Passed: " << passedCount << std::endl;
    std::cout << "❌ Failed: " << failedCount << std::endl;
    std::cout << "Success rate: " << std::fixed << std::setprecision(1) 
              << (static_cast<float>(passedCount) / total * 100.0f) << "%" << std::endl;
    std::cout << "Total time: " << formatDuration(totalElapsedTime) << std::endl;
    
    // Show failed tests
    auto failedTests = getFailedTests();
    if (!failedTests.empty()) {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "FAILED TESTS" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        for (const auto& test : failedTests) {
            std::cout << "\n❌ " << test.testName << std::endl;
            std::cout << "   Status: " << test.statusString() << std::endl;
            std::cout << "   Duration: " << formatDuration(test.elapsedSeconds) << std::endl;
            if (!test.error.empty()) {
                // Show first few lines of error
                std::istringstream errorStream(test.error);
                std::string line;
                int lineCount = 0;
                while (std::getline(errorStream, line) && lineCount < 5) {
                    std::cout << "   " << line << std::endl;
                    lineCount++;
                }
                if (lineCount == 5) {
                    std::cout << "   ..." << std::endl;
                }
            }
        }
    }
}

void CppTestRunner::saveJsonResults() const {
    std::ofstream out(config.jsonOutputFile);
    if (!out.is_open()) return;
    
    out << "{\n";
    
    // Summary
    out << "  \"summary\": {\n";
    out << "    \"total\": " << (passedCount + failedCount) << ",\n";
    out << "    \"passed\": " << passedCount.load() << ",\n";
    out << "    \"failed\": " << failedCount.load() << ",\n";
    out << "    \"success_rate\": " << std::fixed << std::setprecision(1);
    if (passedCount + failedCount > 0) {
        out << (static_cast<float>(passedCount) / (passedCount + failedCount) * 100.0f);
    } else {
        out << "0.0";
    }
    out << ",\n";
    out << "    \"total_time\": " << std::fixed << std::setprecision(3) << totalElapsedTime.load() << ",\n";
    out << "    \"parallel\": " << (config.parallel ? "true" : "false");
    if (config.parallel) {
        out << ",\n    \"workers\": ";
        if (config.maxWorkers == 0) {
            out << "\"auto\"";
        } else {
            out << config.maxWorkers;
        }
    }
    out << "\n  },\n";
    
    // Results array
    out << "  \"results\": [\n";
    bool firstResult = true;
    for (const auto& result : results) {
        if (!firstResult) out << ",\n";
        out << "    {\n";
        out << "      \"test_name\": \"" << json_utils::escape_json(result.testName) << "\",\n";
        out << "      \"test_file\": \"" << json_utils::escape_json(result.testFile) << "\",\n";
        out << "      \"category\": \"" << getCategoryName(result.category) << "\",\n";
        out << "      \"status\": \"" << result.statusString() << "\",\n";
        out << "      \"elapsed_seconds\": " << std::fixed << std::setprecision(3) << result.elapsedSeconds << ",\n";
        out << "      \"return_code\": " << result.returnCode << ",\n";
        out << "      \"worker_id\": " << result.workerId << ",\n";
        out << "      \"timestamp\": " << std::chrono::system_clock::to_time_t(result.timestamp) << ",\n";
        out << "      \"success\": " << (result.isSuccess() ? "true" : "false");
        
        if (!result.output.empty()) {
            out << ",\n      \"output\": \"" << json_utils::escape_json(result.output) << "\"";
        }
        if (!result.error.empty()) {
            out << ",\n      \"error\": \"" << json_utils::escape_json(result.error) << "\"";
        }
        
        out << "\n    }";
        firstResult = false;
    }
    out << "\n  ]";
    
    // Failed tests summary
    if (failedCount > 0) {
        out << ",\n  \"failed_tests\": [\n";
        bool firstFailed = true;
        for (const auto& result : results) {
            if (!result.isSuccess()) {
                if (!firstFailed) out << ",\n";
                out << "    {\n";
                out << "      \"test_name\": \"" << json_utils::escape_json(result.testName) << "\",\n";
                out << "      \"status\": \"" << result.statusString() << "\",\n";
                out << "      \"error\": \"" << json_utils::escape_json(result.error) << "\"\n";
                out << "    }";
                firstFailed = false;
            }
        }
        out << "\n  ]";
    }
    
    out << "\n}\n";
    out.close();
    
    std::cout << "\n📊 Detailed results saved to: " << config.jsonOutputFile << std::endl;
}

void CppTestRunner::saveLogFile() const {
    if (!logFile || !logFile->is_open()) return;
    
    *logFile << "\n" << std::string(80, '=') << "\n";
    *logFile << "FINAL TEST EXECUTION SUMMARY\n";
    *logFile << std::string(80, '=') << "\n";
    *logFile << "End Time: " << getCurrentTimestamp() << "\n";
    *logFile << "Total Duration: " << formatDuration(totalElapsedTime) << "\n";
    *logFile << "Total Tests: " << (passedCount + failedCount) << "\n";
    *logFile << "Passed: " << passedCount << " (" 
             << std::fixed << std::setprecision(1) 
             << (static_cast<float>(passedCount) / (passedCount + failedCount) * 100.0f) << "%)\n";
    *logFile << "Failed: " << failedCount << " (" 
             << std::fixed << std::setprecision(1) 
             << (static_cast<float>(failedCount) / (passedCount + failedCount) * 100.0f) << "%)\n";
    
    // Test execution breakdown
    *logFile << "\nTEST EXECUTION TIME BREAKDOWN\n";
    *logFile << std::string(80, '-') << "\n";
    *logFile << std::setw(40) << std::left << "Test Name" 
             << std::setw(15) << "Category"
             << std::setw(15) << "Status"
             << std::setw(10) << "Time (s)" << "\n";
    *logFile << std::string(80, '-') << "\n";
    
    // Sort by execution time
    std::vector<TestResult> sortedResults = results;
    std::sort(sortedResults.begin(), sortedResults.end(), 
              [](const TestResult& a, const TestResult& b) {
                  return a.elapsedSeconds > b.elapsedSeconds;
              });
    
    for (const auto& result : sortedResults) {
        *logFile << std::setw(40) << std::left << result.testName
                 << std::setw(15) << getCategoryName(result.category)
                 << std::setw(15) << result.statusString()
                 << std::setw(10) << std::fixed << std::setprecision(2) 
                 << result.elapsedSeconds << "\n";
    }
    
    std::cout << "📋 Full details saved to: " << config.logFile << std::endl;
}

void CppTestRunner::logMessage(const std::string& message, const std::string& level) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (logFile && logFile->is_open()) {
        *logFile << "[" << getCurrentTimestamp() << "] [" 
                 << std::setw(8) << std::left << level << "] " 
                 << message << "\n";
        logFile->flush();
    }
    
    if (config.verbose && (level == "ERROR" || level == "WARNING")) {
        std::cerr << "   " << message << std::endl;
    }
}

void CppTestRunner::logTestStart(const TestDefinition& test, int workerId) {
    std::stringstream msg;
    msg << std::string(60, '=') << "\n";
    msg << "TEST START: " << test.name << " (" << currentTestIndex << "/" << tests.size() << ")\n";
    msg << "Type: " << getCategoryName(test.category) << "\n";
    msg << "File: " << test.sourceFile << "\n";
    msg << "Timeout: " << test.timeoutSeconds << " seconds\n";
    if (config.parallel && workerId > 0) {
        msg << "Worker ID: " << workerId << "\n";
    }
    msg << std::string(60, '=');
    
    logMessage(msg.str(), "INFO");
}

void CppTestRunner::logTestResult(const TestResult& result) {
    std::string level = result.isSuccess() ? "SUCCESS" : "ERROR";
    
    std::stringstream msg;
    msg << "TEST " << result.statusString() << ": " << result.testName << "\n";
    msg << "Duration: " << std::fixed << std::setprecision(2) << result.elapsedSeconds << " seconds\n";
    msg << "Return Code: " << result.returnCode;
    
    if (!result.output.empty() && (config.verbose || !result.isSuccess())) {
        msg << "\n" << std::string(40, '=') << " OUTPUT " << std::string(40, '=') << "\n";
        msg << result.output;
    }
    
    if (!result.error.empty()) {
        msg << "\n" << std::string(40, '=') << " ERROR " << std::string(40, '=') << "\n";
        msg << result.error;
    }
    
    msg << "\n" << std::string(60, '=');
    
    logMessage(msg.str(), level);
}

std::string CppTestRunner::getCategoryName(TestCategory category) const {
    switch (category) {
        case TestCategory::UNIT: return "Unit";
        case TestCategory::INTEGRATION: return "Integration";
        case TestCategory::BUILD: return "Build";
        case TestCategory::RESOURCE: return "Resource";
        case TestCategory::THREADING: return "Threading";
        case TestCategory::MEMORY: return "Memory";
        case TestCategory::PERFORMANCE: return "Performance";
        default: return "Unknown";
    }
}

std::string CppTestRunner::getStatusIcon(TestStatus status) const {
    switch (status) {
        case TestStatus::RUNNING: return "⏳";
        case TestStatus::PASSED: return "✅";
        case TestStatus::FAILED: return "❌";
        case TestStatus::TIMEOUT: return "⏱️";
        case TestStatus::COMPILATION_FAILED: return "🔨";
        case TestStatus::SKIPPED: return "⏭️";
        default: return "❓";
    }
}

std::string CppTestRunner::formatDuration(double seconds) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << seconds << "s";
    return ss.str();
}

std::string CppTestRunner::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool CppTestRunner::shouldRunTest(const TestDefinition& test) const {
    // Skip slow tests if requested
    if (config.skipSlowTests && test.timeoutSeconds > 60) {
        return false;
    }
    
    // Check category filter
    if (!config.categoriesToRun.empty()) {
        auto it = std::find(config.categoriesToRun.begin(), config.categoriesToRun.end(), test.category);
        return it != config.categoriesToRun.end();
    }
    
    return true;
}

int CppTestRunner::getOptimalWorkerCount() const {
    // Get hardware concurrency
    unsigned int hw_threads = std::thread::hardware_concurrency();
    if (hw_threads == 0) hw_threads = 4; // Fallback
    
    // Use half the threads for CPU-bound tests, but at least 2
    int optimal = std::max(2u, hw_threads / 2);
    
    // Cap at number of tests
    optimal = std::min(optimal, static_cast<int>(tests.size()));
    
    return optimal;
}

std::vector<TestResult> CppTestRunner::getResults() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(resultsMutex));
    return results;
}

std::vector<TestResult> CppTestRunner::getFailedTests() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(resultsMutex));
    std::vector<TestResult> failed;
    for (const auto& result : results) {
        if (!result.isSuccess()) {
            failed.push_back(result);
        }
    }
    return failed;
}

} // namespace GameEngine::Testing