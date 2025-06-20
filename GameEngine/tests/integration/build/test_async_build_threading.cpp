#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include "build/async_build_system.h"
#include "build/build_system.h"
#include "project/project.h"
#include "utils/path_utils.h"
#include <nlohmann/json.hpp>

using namespace GameEngine;

// Global flag to control test execution
static bool skipBuildTests = false;

// Helper function to create a test project
std::unique_ptr<Project> createTestProject(const std::string& name) {
    // Create project directory
    std::string projectPath = "./test_projects/" + name;
    std::filesystem::create_directories(projectPath);
    std::filesystem::create_directories(projectPath + "/scenes");
    std::filesystem::create_directories(projectPath + "/assets");
    
    // Create project.json
    nlohmann::json projectJson = {
        {"name", name},
        {"version", "1.0.0"},
        {"scenes", nlohmann::json::array()},
        {"metadata", nlohmann::json::object()}
    };
    
    std::ofstream file(projectPath + "/project.json");
    file << projectJson.dump(4);
    file.close();
    
    auto project = std::make_unique<Project>();
    project->load(projectPath);
    return project;
}

// Helper to check if templates exist
bool checkTemplatesExist() {
    // Check if we can find the templates directory
    std::filesystem::path templatesDir = EnginePaths::getTemplatesDir();
    std::filesystem::path gameTemplate = templatesDir / "basic" / "game_template.cpp";
    
    if (!std::filesystem::exists(gameTemplate)) {
        std::cout << "WARNING: Game template not found at: " << gameTemplate << std::endl;
        std::cout << "Templates directory: " << templatesDir << std::endl;
        std::cout << "This test requires the templates directory from the main GameEngine." << std::endl;
        std::cout << "Skipping actual build operations to prevent abort." << std::endl;
        return false;
    }
    return true;
}

void testThreadSafeStartBuild() {
    std::cout << "Testing thread-safe startBuild..." << std::endl;
    
    if (skipBuildTests) {
        std::cout << "✓ Thread-safe startBuild test skipped (no templates)" << std::endl;
        return;
    }
    
    AsyncBuildSystem buildSystem;
    auto mockProject = createTestProject("TestProject");
    
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    std::atomic<int> failureCount{0};
    
    // Start 10 threads trying to build simultaneously
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&buildSystem, &mockProject, &successCount, &failureCount]() {
            if (buildSystem.startBuild(mockProject.get())) {
                successCount.fetch_add(1);
            } else {
                failureCount.fetch_add(1);
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Only one thread should succeed
    assert(successCount.load() == 1);
    assert(failureCount.load() == 9);
    assert(buildSystem.getStatus() == AsyncBuildSystem::BuildStatus::InProgress ||
           buildSystem.getStatus() == AsyncBuildSystem::BuildStatus::Failed);
    
    std::cout << "✓ Thread-safe startBuild test passed" << std::endl;
    std::cout << "  Success count: " << successCount.load() << std::endl;
    std::cout << "  Failure count: " << failureCount.load() << std::endl;
    
    // Wait for build to complete or timeout
    int timeout = 100; // 10 seconds
    while (buildSystem.getStatus() == AsyncBuildSystem::BuildStatus::InProgress && timeout > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        timeout--;
    }
}

void testStatusAccessThreadSafe() {
    std::cout << "\nTesting thread-safe status access..." << std::endl;
    
    AsyncBuildSystem buildSystem;
    std::atomic<bool> stop{false};
    std::atomic<int> readCount{0};
    std::atomic<bool> hasError{false};
    
    // Reader thread - continuously reads status
    std::thread reader([&buildSystem, &stop, &readCount, &hasError]() {
        while (!stop.load()) {
            try {
                AsyncBuildSystem::BuildStatus status = buildSystem.getStatus();
                readCount.fetch_add(1);
                
                // Verify status is a valid enum value
                if (status != AsyncBuildSystem::BuildStatus::Idle &&
                    status != AsyncBuildSystem::BuildStatus::InProgress &&
                    status != AsyncBuildSystem::BuildStatus::Success &&
                    status != AsyncBuildSystem::BuildStatus::Failed) {
                    hasError.store(true);
                    break;
                }
            } catch (...) {
                hasError.store(true);
                break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    });
    
    // Writer thread - changes status multiple times
    std::thread writer([&buildSystem, &hasError]() {
        try {
            for (int i = 0; i < 100; ++i) {  // Reduced iterations to prevent excessive project creation
                if (skipBuildTests) {
                    // Just sleep to simulate work
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } else {
                    // Can't directly set status from outside, so we'll test with multiple build attempts
                    auto mockProject = createTestProject("TestProject" + std::to_string(i));
                    
                    // Try to start a build (will fail if one is already in progress)
                    buildSystem.startBuild(mockProject.get());
                    
                    // Small delay
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
        } catch (...) {
            hasError.store(true);
        }
    });
    
    writer.join();
    stop.store(true);
    reader.join();
    
    // Should not have any errors and should have many reads
    assert(!hasError.load());
    assert(readCount.load() > 0);
    
    std::cout << "✓ Thread-safe status access test passed" << std::endl;
    std::cout << "  Read count: " << readCount.load() << std::endl;
}

void testConcurrentBuildRequests() {
    std::cout << "\nTesting concurrent build requests..." << std::endl;
    
    if (skipBuildTests) {
        std::cout << "✓ Concurrent build requests test skipped (no templates)" << std::endl;
        return;
    }
    
    AsyncBuildSystem buildSystem;
    std::vector<std::thread> threads;
    std::atomic<int> attemptCount{0};
    std::atomic<int> successCount{0};
    
    // Create multiple projects
    std::vector<std::unique_ptr<Project>> projects;
    for (int i = 0; i < 5; ++i) {
        projects.push_back(createTestProject("Project" + std::to_string(i)));
    }
    
    // Launch threads that repeatedly try to start builds
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&buildSystem, &projects, &attemptCount, &successCount, i]() {
            for (int j = 0; j < 20; ++j) {
                attemptCount.fetch_add(1);
                if (buildSystem.startBuild(projects[i].get())) {
                    successCount.fetch_add(1);
                    // Wait a bit to simulate build time
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "✓ Concurrent build requests test passed" << std::endl;
    std::cout << "  Total attempts: " << attemptCount.load() << std::endl;
    std::cout << "  Successful builds: " << successCount.load() << std::endl;
    
    // Success count should be reasonable (not all 100 since builds take time)
    assert(successCount.load() > 0);
    assert(successCount.load() < attemptCount.load());
}

void testRapidStatusChanges() {
    std::cout << "\nTesting rapid status changes..." << std::endl;
    
    if (skipBuildTests) {
        std::cout << "✓ Rapid status changes test skipped (no templates)" << std::endl;
        return;
    }
    
    AsyncBuildSystem buildSystem;
    std::atomic<bool> stop{false};
    std::atomic<int> changeCount{0};
    std::atomic<bool> hasError{false};
    
    // Monitor thread - tracks status changes
    std::thread monitor([&buildSystem, &stop, &changeCount, &hasError]() {
        AsyncBuildSystem::BuildStatus lastStatus = buildSystem.getStatus();
        while (!stop.load()) {
            try {
                AsyncBuildSystem::BuildStatus currentStatus = buildSystem.getStatus();
                if (currentStatus != lastStatus) {
                    changeCount.fetch_add(1);
                    lastStatus = currentStatus;
                }
            } catch (...) {
                hasError.store(true);
                break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    });
    
    // Rapid build starter thread
    std::thread builder([&buildSystem, &hasError]() {
        try {
            for (int i = 0; i < 10; ++i) {  // Reduced iterations
                auto mockProject = createTestProject("RapidProject" + std::to_string(i));
                
                buildSystem.startBuild(mockProject.get());
                
                // Wait for status to change
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                
                // Cancel and wait
                buildSystem.cancelBuild();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        } catch (...) {
            hasError.store(true);
        }
    });
    
    builder.join();
    stop.store(true);
    monitor.join();
    
    assert(!hasError.load());
    
    std::cout << "✓ Rapid status changes test passed" << std::endl;
    std::cout << "  Status changes detected: " << changeCount.load() << std::endl;
}

int main() {
    std::cout << "Running AsyncBuildSystem threading tests...\n" << std::endl;
    
    // Create test directories
    std::filesystem::create_directories("./test_projects");
    
    // Check if we have access to templates
    skipBuildTests = !checkTemplatesExist();
    
    try {
        testThreadSafeStartBuild();
        testStatusAccessThreadSafe();
        testConcurrentBuildRequests();
        testRapidStatusChanges();
        
        std::cout << "\n✅ All threading tests passed!" << std::endl;
        if (skipBuildTests) {
            std::cout << "Note: Some tests were skipped due to missing templates." << std::endl;
            std::cout << "This is expected when running tests in isolation." << std::endl;
        }
        
        // Clean up test projects
        std::filesystem::remove_all("./test_projects");
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << std::endl;
        
        // Clean up on failure too
        try {
            std::filesystem::remove_all("./test_projects");
        } catch (...) {}
        
        return 1;
    }
}