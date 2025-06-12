#ifndef CPP_TEST_RUNNER_H
#define CPP_TEST_RUNNER_H

#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <map>
#include <optional>

namespace GameEngine::Testing {

// Test categories matching Python test categories
enum class TestCategory {
    UNIT,        // Fast unit tests
    INTEGRATION, // Integration tests
    BUILD,       // Build system tests
    RESOURCE,    // Resource manager tests
    THREADING,   // Threading/concurrency tests
    MEMORY,      // Memory safety tests
    PERFORMANCE  // Performance tests
};

// Test result status
enum class TestStatus {
    PENDING,
    RUNNING,
    PASSED,
    FAILED,
    TIMEOUT,
    COMPILATION_FAILED,
    SKIPPED
};

// Test result structure
struct TestResult {
    std::string testName;
    std::string testFile;
    TestCategory category;
    TestStatus status;
    double elapsedSeconds;
    std::string output;
    std::string error;
    int returnCode;
    int workerId;
    std::chrono::system_clock::time_point timestamp;
    
    TestResult() : status(TestStatus::PENDING), elapsedSeconds(0), returnCode(0), workerId(0) {}
    
    bool isSuccess() const { return status == TestStatus::PASSED; }
    std::string statusString() const;
    std::string toJson() const;
};

// Test definition
struct TestDefinition {
    std::string name;
    std::string sourceFile;
    TestCategory category;
    std::vector<std::string> additionalSources; // Extra source files to compile
    std::vector<std::string> additionalFlags;   // Extra compile flags
    int timeoutSeconds;
    bool requiresDisplay;  // Some tests may need display
    
    TestDefinition(const std::string& name, const std::string& file, TestCategory cat, int timeout = 30)
        : name(name), sourceFile(file), category(cat), timeoutSeconds(timeout), requiresDisplay(false) {}
};

// Progress callback for UI updates
using ProgressCallback = std::function<void(int current, int total, const TestResult& result)>;

// Test runner configuration
struct TestRunnerConfig {
    bool verbose = false;
    bool showProgress = true;
    bool parallel = false;
    int maxWorkers = 0;  // 0 = auto-detect
    bool skipSlowTests = false;
    std::string logFile;
    std::string jsonOutputFile = "cpp_test_results.json";
    std::vector<TestCategory> categoriesToRun;  // Empty = run all
    
    // Compilation settings
    std::string compiler = "g++";
    std::string cppStandard = "-std=c++20";
    std::vector<std::string> includePaths;
    std::vector<std::string> libraryPaths;
    std::vector<std::string> libraries;
    std::vector<std::string> frameworks;  // For macOS
};

// Main test runner class
class CppTestRunner {
public:
    explicit CppTestRunner(const TestRunnerConfig& config);
    ~CppTestRunner();
    
    // Register tests
    void registerTest(const TestDefinition& test);
    void registerAllDefaultTests();  // Register all known tests
    
    // Run tests
    void runAll();
    void runCategory(TestCategory category);
    void runTest(const std::string& testName);
    
    // Results
    int getPassedCount() const { return passedCount; }
    int getFailedCount() const { return failedCount; }
    int getTotalCount() const { return static_cast<int>(tests.size()); }
    double getTotalElapsedTime() const { return totalElapsedTime; }
    std::vector<TestResult> getResults() const;
    std::vector<TestResult> getFailedTests() const;
    
    // Progress
    void setProgressCallback(ProgressCallback callback) { progressCallback = callback; }
    
    // Output
    void printSummary() const;
    void saveJsonResults() const;
    void saveLogFile() const;

private:
    // Test execution
    TestResult compileAndRunTest(const TestDefinition& test, int workerId = 0);
    TestResult compileTest(const TestDefinition& test);
    TestResult runCompiledTest(const TestDefinition& test, const std::string& executablePath);
    
    // Parallel execution
    void runTestsParallel();
    void runTestsSequential();
    
    // Compilation
    std::string buildCompileCommand(const TestDefinition& test) const;
    std::vector<std::string> getIncludeFlags() const;
    std::vector<std::string> getLibraryFlags() const;
    
    // Logging
    void logMessage(const std::string& message, const std::string& level = "INFO");
    void logTestStart(const TestDefinition& test, int workerId);
    void logTestResult(const TestResult& result);
    
    // Progress
    void updateProgress(const TestResult& result);
    void printProgressBar(int current, int total, const TestResult& result);
    
    // Utilities
    std::string getCategoryName(TestCategory category) const;
    std::string getStatusIcon(TestStatus status) const;
    std::string formatDuration(double seconds) const;
    std::string getCurrentTimestamp() const;
    bool shouldRunTest(const TestDefinition& test) const;
    int getOptimalWorkerCount() const;
    
private:
    TestRunnerConfig config;
    std::vector<TestDefinition> tests;
    std::vector<TestResult> results;
    
    std::atomic<int> passedCount{0};
    std::atomic<int> failedCount{0};
    std::atomic<int> currentTestIndex{0};
    std::atomic<double> totalElapsedTime{0};
    
    std::mutex resultsMutex;
    std::mutex logMutex;
    
    ProgressCallback progressCallback;
    std::chrono::system_clock::time_point startTime;
    
    // Log file handle
    std::unique_ptr<std::ofstream> logFile;
};

} // namespace GameEngine::Testing

#endif // CPP_TEST_RUNNER_H