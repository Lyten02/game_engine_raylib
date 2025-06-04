#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

class ScriptManager {
private:
    lua_State* L = nullptr;
    std::unordered_map<std::string, std::string> loadedScripts;
    bool initialized = false;
    
    // Error handling
    void reportError(const std::string& context);
    
public:
    ScriptManager() = default;
    ~ScriptManager();
    
    // Initialize Lua state and standard libraries
    bool initialize();
    
    // Shutdown and cleanup
    void shutdown();
    
    // Execute a Lua script from file
    bool executeScript(const std::string& scriptPath);
    
    // Execute a Lua string
    bool executeString(const std::string& luaCode);
    
    // Register engine bindings (Vector3, Transform, logging, etc.)
    void registerEngineBindings();
    
    // Reload and execute a script
    void reloadScript(const std::string& scriptPath);
    
    // Check if a script is loaded
    bool isScriptLoaded(const std::string& scriptPath) const;
    
    // Get loaded scripts list
    std::vector<std::string> getLoadedScripts() const;
    
    // Call a Lua function by name
    bool callFunction(const std::string& functionName, int numArgs = 0, int numResults = 0);
    
    // Get Lua state (for advanced usage)
    lua_State* getLuaState() { return L; }
    
    // Check if initialized
    bool isInitialized() const { return initialized; }
};