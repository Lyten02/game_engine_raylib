#include "script_manager.h"
#include "lua_bindings.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <sstream>

ScriptManager::~ScriptManager() {
    shutdown();
}

bool ScriptManager::initialize() {
    if (initialized) {
        spdlog::warn("ScriptManager::initialize - Already initialized");
        return true;
    }
    
    // Create new Lua state
    L = luaL_newstate();
    if (!L) {
        spdlog::error("ScriptManager::initialize - Failed to create Lua state");
        return false;
    }
    
    // Open standard libraries
    luaL_openlibs(L);
    
    // Register engine bindings
    registerEngineBindings();
    
    initialized = true;
    spdlog::info("ScriptManager::initialize - Lua scripting initialized");
    
    return true;
}

void ScriptManager::shutdown() {
    if (L) {
        lua_close(L);
        L = nullptr;
    }
    
    loadedScripts.clear();
    initialized = false;
    
    spdlog::info("ScriptManager::shutdown - Lua scripting shut down");
}

bool ScriptManager::executeScript(const std::string& scriptPath) {
    if (!initialized) {
        spdlog::error("ScriptManager::executeScript - Not initialized");
        return false;
    }
    
    if (!std::filesystem::exists(scriptPath)) {
        spdlog::error("ScriptManager::executeScript - Script not found: {}", scriptPath);
        return false;
    }
    
    // Read script file
    std::ifstream file(scriptPath);
    if (!file.is_open()) {
        spdlog::error("ScriptManager::executeScript - Failed to open script: {}", scriptPath);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string scriptContent = buffer.str();
    
    // Store loaded script
    loadedScripts[scriptPath] = scriptContent;
    
    // Execute script
    int result = luaL_dostring(L, scriptContent.c_str());
    if (result != LUA_OK) {
        reportError("executeScript: " + scriptPath);
        return false;
    }
    
    spdlog::info("ScriptManager::executeScript - Executed script: {}", scriptPath);
    return true;
}

bool ScriptManager::executeString(const std::string& luaCode) {
    if (!initialized) {
        spdlog::error("ScriptManager::executeString - Not initialized");
        return false;
    }
    
    int result = luaL_dostring(L, luaCode.c_str());
    if (result != LUA_OK) {
        reportError("executeString");
        return false;
    }
    
    return true;
}

void ScriptManager::registerEngineBindings() {
    // Register Vector3
    registerVector3(L);
    
    // Register Transform
    registerTransform(L);
    
    // Register logging functions
    registerLogging(L);
    
    spdlog::debug("ScriptManager::registerEngineBindings - Engine bindings registered");
}

void ScriptManager::reloadScript(const std::string& scriptPath) {
    if (isScriptLoaded(scriptPath)) {
        spdlog::info("ScriptManager::reloadScript - Reloading script: {}", scriptPath);
        executeScript(scriptPath);
    } else {
        spdlog::warn("ScriptManager::reloadScript - Script not loaded: {}", scriptPath);
        executeScript(scriptPath);
    }
}

bool ScriptManager::isScriptLoaded(const std::string& scriptPath) const {
    return loadedScripts.find(scriptPath) != loadedScripts.end();
}

std::vector<std::string> ScriptManager::getLoadedScripts() const {
    std::vector<std::string> scripts;
    for (const auto& [path, content] : loadedScripts) {
        scripts.push_back(path);
    }
    return scripts;
}

bool ScriptManager::callFunction(const std::string& functionName, int numArgs, int numResults) {
    if (!initialized) {
        spdlog::error("ScriptManager::callFunction - Not initialized");
        return false;
    }
    
    // Get function from global table
    lua_getglobal(L, functionName.c_str());
    
    if (!lua_isfunction(L, -1 - numArgs)) {
        lua_pop(L, numArgs + 1);  // Pop function and arguments
        spdlog::error("ScriptManager::callFunction - Function not found: {}", functionName);
        return false;
    }
    
    // Call the function
    if (lua_pcall(L, numArgs, numResults, 0) != LUA_OK) {
        reportError("callFunction: " + functionName);
        return false;
    }
    
    return true;
}

void ScriptManager::reportError(const std::string& context) {
    if (!L) return;
    
    const char* error = lua_tostring(L, -1);
    spdlog::error("ScriptManager - Lua error in {}: {}", context, error ? error : "Unknown error");
    lua_pop(L, 1);  // Pop error message
}