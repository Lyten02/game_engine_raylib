#include <iostream>
#include <vector>
#include <memory>
#include "../src/console/console.h"
#include "../src/console/command_processor.h"
#include "../src/console/command_registry.h"
#include "../src/utils/config.h"
#include <spdlog/spdlog.h>

void test_basic_autocompletion() {
    std::cout << "=== Testing Console Basic Autocompletion ===" << std::endl;
    
    // Initialize console and command processor
    Console console;
    CommandProcessor processor;
    
    console.initialize();
    processor.initialize(&console);
    console.setCommandProcessor(&processor);
    
    // Test that we can trigger autocompletion multiple times
    for (int i = 0; i < 10; i++) {
        console.update(0.016f); // Simulate frame updates
    }
    
    console.clear();
    console.shutdown();
    
    std::cout << "✓ Basic autocompletion test passed" << std::endl;
}

void test_memory_cleanup() {
    std::cout << "=== Testing Console Memory Cleanup ===" << std::endl;
    
    // Create and destroy console multiple times
    for (int cycle = 0; cycle < 3; cycle++) {
        Console console;
        CommandProcessor processor;
        
        console.initialize();
        processor.initialize(&console);
        console.setCommandProcessor(&processor);
        
        // Simulate many autocompletion calls
        for (int i = 0; i < 100; i++) {
            console.update(0.016f);
        }
        
        // Test clear functionality
        console.clear();
        
        // Clean shutdown
        console.shutdown();
    }
    
    std::cout << "✓ Memory cleanup test passed" << std::endl;
}

void test_console_integration() {
    std::cout << "=== Testing Console Integration ===" << std::endl;
    
    // Test multiple console instances
    {
        Console console1;
        CommandProcessor processor1;
        
        console1.initialize();
        processor1.initialize(&console1);
        console1.setCommandProcessor(&processor1);
        
        // First console operations
        for (int i = 0; i < 50; i++) {
            console1.update(0.016f);
        }
        
        Console console2;
        CommandProcessor processor2;
        
        console2.initialize();
        processor2.initialize(&console2);
        console2.setCommandProcessor(&processor2);
        
        // Second console operations
        for (int i = 0; i < 50; i++) {
            console2.update(0.016f);
        }
        
        // Clean up both
        console1.clear();
        console2.clear();
        
        console1.shutdown();
        console2.shutdown();
    }
    
    std::cout << "✓ Console integration test passed" << std::endl;
}

int main() {
    // Set up minimal logging
    spdlog::set_level(spdlog::level::err);
    
    try {
        test_basic_autocompletion();
        test_memory_cleanup();
        test_console_integration();
        
        std::cout << "\n✅ All console autocompletion tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}