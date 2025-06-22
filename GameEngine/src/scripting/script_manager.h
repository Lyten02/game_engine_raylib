#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

// Stub class for ScriptManager - Lua scripting has been replaced with C++ GameLogicManager
class ScriptManager {
private:
    bool initialized = false;

public:
    ScriptManager() = default;
    ~ScriptManager() = default;

    // Initialize (no-op)
    bool initialize() { initialized = true; return true; }

    // Shutdown (no-op)
    void shutdown() { initialized = false; }

    // Execute a script (no-op, returns false)
    bool executeScript(const std::string& scriptPath) { return false; }

    // Execute a string (no-op, returns false)
    bool executeString(const std::string& luaCode) { return false; }

    // Register engine bindings (no-op)
    void registerEngineBindings() {}

    // Reload and execute a script (no-op)
    void reloadScript(const std::string& scriptPath) {}

    // Check if a script is loaded (always returns false)
    bool isScriptLoaded(const std::string& scriptPath) const { return false; }

    // Get loaded scripts list (always empty)
    std::vector<std::string> getLoadedScripts() const { return {}; }

    // Call a function (no-op, returns false)
    bool callFunction(const std::string& functionName, int numArgs = 0, int numResults = 0) { return false; }

    // Check if initialized
    bool isInitialized() const { return initialized; }
};
