#include "../src/scripting/script_manager.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <memory>

// Simple test framework
#define EXPECT_TRUE(cond) \
    if (!(cond)) { \
        std::cerr << "Test failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

#define EXPECT_FALSE(cond) \
    if (cond) { \
        std::cerr << "Test failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

// Test ScriptManager without initialization
bool testUninitializedScriptManager() {
    std::cout << "Testing uninitialized ScriptManager..." << std::endl;
    
    ScriptManager manager;
    
    // All operations should fail gracefully
    EXPECT_FALSE(manager.executeScript("nonexistent.lua"));
    EXPECT_FALSE(manager.executeString("print('hello')"));
    EXPECT_FALSE(manager.callFunction("testFunc", 0, 0));
    
    std::cout << "✓ Uninitialized ScriptManager test passed" << std::endl;
    return true;
}

// Test normal operation
bool testNormalOperation() {
    std::cout << "Testing normal ScriptManager operation..." << std::endl;
    
    ScriptManager manager;
    EXPECT_TRUE(manager.initialize());
    
    // Create a simple test script
    const char* testScriptPath = "test_script_temp.lua";
    std::ofstream testScript(testScriptPath);
    testScript << "function testFunc()\n";
    testScript << "  return 42\n";
    testScript << "end\n";
    testScript << "print('Test script loaded')\n";
    testScript.close();
    
    // Execute the script
    EXPECT_TRUE(manager.executeScript(testScriptPath));
    
    // Execute a string
    EXPECT_TRUE(manager.executeString("x = 10"));
    
    // Call the function (no args, no results for simplicity)
    EXPECT_TRUE(manager.executeString("function simpleFunc() end"));
    EXPECT_TRUE(manager.callFunction("simpleFunc", 0, 0));
    
    // Clean up
    std::remove(testScriptPath);
    
    std::cout << "✓ Normal operation test passed" << std::endl;
    return true;
}

// Test error handling
bool testErrorHandling() {
    std::cout << "Testing error handling..." << std::endl;
    
    ScriptManager manager;
    EXPECT_TRUE(manager.initialize());
    
    // Try to execute non-existent script
    EXPECT_FALSE(manager.executeScript("nonexistent_script.lua"));
    
    // Try to execute invalid Lua code
    EXPECT_FALSE(manager.executeString("invalid lua code {{{"));
    
    // Try to call non-existent function
    EXPECT_FALSE(manager.callFunction("nonExistentFunction", 0, 0));
    
    std::cout << "✓ Error handling test passed" << std::endl;
    return true;
}

// Test multiple initialization attempts
bool testMultipleInitialization() {
    std::cout << "Testing multiple initialization..." << std::endl;
    
    ScriptManager manager;
    
    // First initialization should succeed
    EXPECT_TRUE(manager.initialize());
    
    // Second initialization should also return true (already initialized)
    EXPECT_TRUE(manager.initialize());
    
    // Should still work after multiple init attempts
    EXPECT_TRUE(manager.executeString("y = 20"));
    
    std::cout << "✓ Multiple initialization test passed" << std::endl;
    return true;
}

// Test shutdown and reinitialization
bool testShutdownAndReinit() {
    std::cout << "Testing shutdown and reinitialization..." << std::endl;
    
    ScriptManager manager;
    
    // Initialize
    EXPECT_TRUE(manager.initialize());
    EXPECT_TRUE(manager.executeString("z = 30"));
    
    // Shutdown
    manager.shutdown();
    
    // Operations should fail after shutdown
    EXPECT_FALSE(manager.executeString("a = 40"));
    
    // Reinitialize
    EXPECT_TRUE(manager.initialize());
    
    // Should work again
    EXPECT_TRUE(manager.executeString("b = 50"));
    
    std::cout << "✓ Shutdown and reinitialization test passed" << std::endl;
    return true;
}

// Test script reloading
bool testScriptReloading() {
    std::cout << "Testing script reloading..." << std::endl;
    
    ScriptManager manager;
    EXPECT_TRUE(manager.initialize());
    
    const char* reloadScriptPath = "reload_test.lua";
    
    // Create initial script
    std::ofstream script(reloadScriptPath);
    script << "value = 100\n";
    script.close();
    
    // Load script
    EXPECT_TRUE(manager.executeScript(reloadScriptPath));
    EXPECT_TRUE(manager.isScriptLoaded(reloadScriptPath));
    
    // Modify script
    script.open(reloadScriptPath);
    script << "value = 200\n";
    script.close();
    
    // Reload script
    manager.reloadScript(reloadScriptPath);
    
    // Clean up
    std::remove(reloadScriptPath);
    
    std::cout << "✓ Script reloading test passed" << std::endl;
    return true;
}

int main() {
    std::cout << "Running ScriptManager null safety tests...\n" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    // Run all tests
    if (testUninitializedScriptManager()) passed++; else failed++;
    if (testNormalOperation()) passed++; else failed++;
    if (testErrorHandling()) passed++; else failed++;
    if (testMultipleInitialization()) passed++; else failed++;
    if (testShutdownAndReinit()) passed++; else failed++;
    if (testScriptReloading()) passed++; else failed++;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Tests passed: " << passed << std::endl;
    std::cout << "Tests failed: " << failed << std::endl;
    std::cout << "========================================" << std::endl;
    
    return failed > 0 ? 1 : 0;
}