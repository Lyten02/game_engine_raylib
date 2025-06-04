#pragma once

extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

// Register Vector3 type and operations
void registerVector3(lua_State* L);

// Register Transform component
void registerTransform(lua_State* L);

// Register logging functions
void registerLogging(lua_State* L);

// Helper functions for Lua stack operations
void pushVector3(lua_State* L, float x, float y, float z);
bool checkVector3(lua_State* L, int index, float& x, float& y, float& z);