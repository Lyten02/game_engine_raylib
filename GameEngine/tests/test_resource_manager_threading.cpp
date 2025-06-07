#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include "../src/resources/resource_manager.h"

std::atomic<int> errors{0};
std::atomic<int> operations{0};

// Test concurrent texture access
void testConcurrentAccess() {
    std::cout << "Testing concurrent access..." << std::endl;
    
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    const int numThreads = 10;
    const int operationsPerThread = 1000;
    std::vector<std::thread> threads;
    
    auto worker = [&rm, operationsPerThread]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 99);
        
        for (int i = 0; i < operationsPerThread; i++) {
            std::string name = "texture_" + std::to_string(dis(gen));
            Texture2D* tex = rm.getTexture(name);
            
            if (!tex) {
                errors++;
                std::cerr << "Error: getTexture returned nullptr!" << std::endl;
            }
            operations++;
        }
    };
    
    // Start threads
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(worker);
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    if (errors > 0) {
        std::cerr << "FAIL: " << errors << " errors occurred during concurrent access!" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: " << operations << " concurrent operations completed in " 
              << duration << "ms with no errors" << std::endl;
}

// Test concurrent load and unload
void testConcurrentLoadAndUnload() {
    std::cout << "Testing concurrent load and unload..." << std::endl;
    
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    const int numThreads = 8;
    const int operationsPerThread = 500;
    std::vector<std::thread> threads;
    errors = 0;
    operations = 0;
    
    auto loader = [&rm, operationsPerThread]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 49);
        
        for (int i = 0; i < operationsPerThread; i++) {
            std::string name = "tex_" + std::to_string(dis(gen));
            std::string path = "/fake/path/" + name + ".png";
            
            // Randomly choose operation
            int op = dis(gen) % 3;
            switch (op) {
                case 0: {
                    // Load texture
                    Texture2D* tex = rm.loadTexture(path, name);
                    if (!tex) {
                        errors++;
                        std::cerr << "Error: loadTexture returned nullptr!" << std::endl;
                    }
                    break;
                }
                case 1: {
                    // Get texture
                    Texture2D* tex = rm.getTexture(name);
                    if (!tex) {
                        errors++;
                        std::cerr << "Error: getTexture returned nullptr!" << std::endl;
                    }
                    break;
                }
                case 2: {
                    // Unload texture
                    rm.unloadTexture(name);
                    break;
                }
            }
            operations++;
        }
    };
    
    // Start threads
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(loader);
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    if (errors > 0) {
        std::cerr << "FAIL: " << errors << " errors occurred during concurrent load/unload!" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: " << operations << " concurrent load/unload operations completed in " 
              << duration << "ms with no errors" << std::endl;
}

// Test default texture initialization race
void testDefaultTextureInitialization() {
    std::cout << "Testing default texture initialization race..." << std::endl;
    
    const int numThreads = 20;
    std::vector<std::thread> threads;
    std::vector<Texture2D*> results(numThreads);
    
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    std::atomic<bool> start{false};
    
    auto worker = [&rm, &start](Texture2D** result) {
        // Wait for signal to start
        while (!start) {
            std::this_thread::yield();
        }
        
        // All threads try to get texture at the same time
        *result = rm.getTexture("test");
    };
    
    // Create threads
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(worker, &results[i]);
    }
    
    // Start all threads at once
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    start = true;
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    // Check results
    Texture2D* firstResult = results[0];
    if (!firstResult) {
        std::cerr << "FAIL: First thread got nullptr!" << std::endl;
        exit(1);
    }
    
    for (int i = 1; i < numThreads; i++) {
        if (results[i] != firstResult) {
            std::cerr << "FAIL: Thread " << i << " got different texture pointer!" << std::endl;
            exit(1);
        }
    }
    
    std::cout << "PASS: All threads got the same default texture" << std::endl;
}

// Test concurrent operations with unloadAll
void testConcurrentWithUnloadAll() {
    std::cout << "Testing concurrent operations with unloadAll..." << std::endl;
    
    ResourceManager rm;
    rm.setSilentMode(true);
    rm.setHeadlessMode(true);
    rm.setRayLibInitialized(false);
    
    std::atomic<bool> running{true};
    errors = 0;
    
    // Thread that constantly loads and gets textures
    auto worker = [&rm, &running]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 19);
        
        while (running) {
            std::string name = "tex_" + std::to_string(dis(gen));
            Texture2D* tex = rm.getTexture(name);
            if (!tex) {
                errors++;
            }
        }
    };
    
    // Thread that periodically calls unloadAll
    auto unloader = [&rm, &running]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            rm.unloadAll();
        }
    };
    
    // Start threads
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; i++) {
        workers.emplace_back(worker);
    }
    std::thread unloadThread(unloader);
    
    // Run for a while
    std::this_thread::sleep_for(std::chrono::seconds(2));
    running = false;
    
    // Wait for threads
    for (auto& t : workers) {
        t.join();
    }
    unloadThread.join();
    
    if (errors > 0) {
        std::cerr << "FAIL: " << errors << " errors occurred during concurrent unloadAll!" << std::endl;
        exit(1);
    }
    
    std::cout << "PASS: Concurrent operations with unloadAll completed successfully" << std::endl;
}

int main() {
    std::cout << "=== ResourceManager Threading Tests ===" << std::endl;
    
    testConcurrentAccess();
    testConcurrentLoadAndUnload();
    testDefaultTextureInitialization();
    testConcurrentWithUnloadAll();
    
    std::cout << "\nAll threading tests passed!" << std::endl;
    return 0;
}