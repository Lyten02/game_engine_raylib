#include "../src/build/async_build_system.h"
#include "../src/project/project.h"
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include "nlohmann/json.hpp"

using namespace GameEngine;

// Simple test framework macros
#define ASSERT(condition) \
    if (!(condition)) { \
        std::cerr << "Assertion failed: " #condition " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cerr << "Assertion failed: " #a " == " #b " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Expected: " << static_cast<int>(b) << std::endl; \
        std::cerr << "  Actual: " << static_cast<int>(a) << std::endl; \
        return false; \
    }

#define ASSERT_GT(a, b) \
    if (!((a) > (b))) { \
        std::cerr << "Assertion failed: " #a " > " #b " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "  Left: " << (a) << std::endl; \
        std::cerr << "  Right: " << (b) << std::endl; \
        return false; \
    }

// Helper to create a test project
std::unique_ptr<Project> createTestProject(const std::string& name) {
    // Create project directory
    std::filesystem::path testDir = std::filesystem::temp_directory_path() / "async_build_test" / name;
    std::filesystem::create_directories(testDir);
    
    // Create project.json
    nlohmann::json projectJson;
    projectJson["name"] = name;
    projectJson["version"] = "1.0.0";
    projectJson["scenes"] = nlohmann::json::array();
    
    std::ofstream file(testDir / "project.json");
    file << projectJson.dump(4);
    file.close();
    
    // Load project
    auto project = std::make_unique<Project>();
    project->load(testDir.string());
    return project;
}

bool test_concurrent_start_build_requests() {
    std::cout << "Testing concurrent start build requests..." << std::endl;
    
    auto buildSystem = std::make_unique<AsyncBuildSystem>();
    auto project = createTestProject("TestProject");
    
    std::atomic<int> successCount{0};
    std::atomic<int> failCount{0};
    const int threadCount = 10;
    
    std::vector<std::thread> threads;
    
    // Start multiple threads trying to start builds simultaneously
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&buildSystem, &project, &successCount, &failCount]() {
            if (buildSystem->startBuild(project.get(), "Debug")) {
                successCount++;
            } else {
                failCount++;
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Only one build should succeed
    ASSERT_EQ(successCount.load(), 1);
    ASSERT_EQ(failCount.load(), threadCount - 1);
    
    // Wait for the build to complete
    buildSystem->waitForCompletion();
    
    return true;
}

bool test_start_build_while_previous_in_progress() {
    std::cout << "Testing start build while previous in progress..." << std::endl;
    
    auto buildSystem = std::make_unique<AsyncBuildSystem>();
    auto project = createTestProject("TestProject");
    
    // Start first build
    ASSERT(buildSystem->startBuild(project.get(), "Debug"));
    
    // Immediately try to start another build
    ASSERT(!buildSystem->startBuild(project.get(), "Debug"));
    
    // Status should still be InProgress
    ASSERT_EQ(buildSystem->getStatus(), AsyncBuildSystem::BuildStatus::InProgress);
    
    buildSystem->waitForCompletion();
    
    return true;
}

bool test_race_condition_between_check_and_join() {
    std::cout << "Testing race condition between check and join..." << std::endl;
    
    std::atomic<bool> raceDetected{false};
    const int iterations = 50; // Reduced for faster testing
    
    for (int i = 0; i < iterations; ++i) {
        auto localBuildSystem = std::make_unique<AsyncBuildSystem>();
        auto project = createTestProject("TestProject" + std::to_string(i));
        
        // Start a build
        localBuildSystem->startBuild(project.get(), "Debug");
        
        // Create threads that will try to access build status
        std::thread checker([&localBuildSystem, &raceDetected]() {
            try {
                for (int j = 0; j < 10; ++j) {
                    auto status = localBuildSystem->getStatus();
                    auto progress = localBuildSystem->getProgress();
                    auto step = localBuildSystem->getCurrentStep();
                    
                    if (localBuildSystem->hasMessages()) {
                        localBuildSystem->getNextMessage();
                    }
                    
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            } catch (...) {
                raceDetected = true;
            }
        });
        
        // Another thread trying to start new builds
        std::thread builder([&localBuildSystem, &project, &raceDetected]() {
            try {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                localBuildSystem->startBuild(project.get(), "Release");
            } catch (...) {
                raceDetected = true;
            }
        });
        
        checker.join();
        builder.join();
        
        localBuildSystem->cancelBuild();
    }
    
    ASSERT(!raceDetected.load());
    
    return true;
}

bool test_current_step_thread_safety() {
    std::cout << "Testing current step thread safety..." << std::endl;
    
    auto buildSystem = std::make_unique<AsyncBuildSystem>();
    auto project = createTestProject("TestProject");
    
    const int readerCount = 5;
    std::atomic<bool> inconsistencyDetected{false};
    std::atomic<bool> stopReading{false};
    
    // Start a build
    ASSERT(buildSystem->startBuild(project.get(), "Debug"));
    
    // Start multiple reader threads
    std::vector<std::thread> readers;
    for (int i = 0; i < readerCount; ++i) {
        readers.emplace_back([&buildSystem, &inconsistencyDetected, &stopReading]() {
            while (!stopReading.load()) {
                try {
                    std::string step = buildSystem->getCurrentStep();
                    float progress = buildSystem->getProgress();
                    
                    // Basic sanity check - progress should match step expectations
                    if (progress > 0.9f && step.find("Starting") != std::string::npos) {
                        inconsistencyDetected = true;
                    }
                } catch (...) {
                    inconsistencyDetected = true;
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    // Let readers run while build progresses
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    stopReading = true;
    for (auto& t : readers) {
        t.join();
    }
    
    ASSERT(!inconsistencyDetected.load());
    
    buildSystem->waitForCompletion();
    
    return true;
}

bool test_destructor_while_build_in_progress() {
    std::cout << "Testing destructor while build in progress..." << std::endl;
    
    auto localBuildSystem = std::make_unique<AsyncBuildSystem>();
    auto project = createTestProject("TestProject");
    
    // Start a build
    ASSERT(localBuildSystem->startBuild(project.get(), "Debug"));
    
    // Destroy the build system while build is in progress
    // This should not crash or hang
    localBuildSystem.reset();
    
    // If we get here without crashing, the test passes
    return true;
}

bool test_message_queue_thread_safety() {
    std::cout << "Testing message queue thread safety..." << std::endl;
    
    auto buildSystem = std::make_unique<AsyncBuildSystem>();
    auto project = createTestProject("TestProject");
    
    std::atomic<int> messageCount{0};
    std::atomic<bool> stopReading{false};
    const int readerThreads = 3;
    
    // Start a build
    ASSERT(buildSystem->startBuild(project.get(), "Debug"));
    
    // Start reader threads
    std::vector<std::thread> readers;
    for (int i = 0; i < readerThreads; ++i) {
        readers.emplace_back([&buildSystem, &messageCount, &stopReading]() {
            while (!stopReading.load()) {
                if (buildSystem->hasMessages()) {
                    std::string msg = buildSystem->getNextMessage();
                    if (!msg.empty()) {
                        messageCount++;
                    }
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }
    
    // Let the build run
    buildSystem->waitForCompletion();
    
    // Stop readers
    stopReading = true;
    for (auto& t : readers) {
        t.join();
    }
    
    // We should have received some messages
    ASSERT_GT(messageCount.load(), 0);
    
    return true;
}

int main() {
    std::cout << "Running AsyncBuildSystem thread safety tests..." << std::endl;
    std::cout << "=============================================" << std::endl;
    
    // Clean up any previous test directories
    try {
        std::filesystem::remove_all(std::filesystem::temp_directory_path() / "async_build_test");
    } catch (...) {
        // Ignore errors
    }
    
    int passed = 0;
    int failed = 0;
    
    // Run all tests
    struct Test {
        const char* name;
        bool (*func)();
    };
    
    Test tests[] = {
        {"ConcurrentStartBuildRequests", test_concurrent_start_build_requests},
        {"StartBuildWhilePreviousInProgress", test_start_build_while_previous_in_progress},
        {"RaceConditionBetweenCheckAndJoin", test_race_condition_between_check_and_join},
        {"CurrentStepThreadSafety", test_current_step_thread_safety},
        {"DestructorWhileBuildInProgress", test_destructor_while_build_in_progress},
        {"MessageQueueThreadSafety", test_message_queue_thread_safety}
    };
    
    for (const auto& test : tests) {
        std::cout << "\nRunning test: " << test.name << std::endl;
        
        try {
            if (test.func()) {
                std::cout << "✅ PASSED" << std::endl;
                passed++;
            } else {
                std::cout << "❌ FAILED" << std::endl;
                failed++;
            }
        } catch (const std::exception& e) {
            std::cout << "❌ FAILED with exception: " << e.what() << std::endl;
            failed++;
        } catch (...) {
            std::cout << "❌ FAILED with unknown exception" << std::endl;
            failed++;
        }
    }
    
    std::cout << "\n=============================================" << std::endl;
    std::cout << "Total tests: " << (passed + failed) << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    return failed > 0 ? 1 : 0;
}