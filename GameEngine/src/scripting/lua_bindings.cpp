#include "lua_bindings.h"
#include <spdlog/spdlog.h>
#include <string>

// Vector3 metatable name
static const char* VECTOR3_MT = "Vector3";

// Vector3 methods
static int vector3_new(lua_State* L) {
    float x = luaL_optnumber(L, 1, 0.0);
    float y = luaL_optnumber(L, 2, 0.0);
    float z = luaL_optnumber(L, 3, 0.0);
    
    pushVector3(L, x, y, z);
    return 1;
}

static int vector3_index(lua_State* L) {
    float* v = (float*)luaL_checkudata(L, 1, VECTOR3_MT);
    const char* key = luaL_checkstring(L, 2);
    
    if (strcmp(key, "x") == 0) {
        lua_pushnumber(L, v[0]);
    } else if (strcmp(key, "y") == 0) {
        lua_pushnumber(L, v[1]);
    } else if (strcmp(key, "z") == 0) {
        lua_pushnumber(L, v[2]);
    } else {
        lua_pushnil(L);
    }
    
    return 1;
}

static int vector3_newindex(lua_State* L) {
    float* v = (float*)luaL_checkudata(L, 1, VECTOR3_MT);
    const char* key = luaL_checkstring(L, 2);
    float value = luaL_checknumber(L, 3);
    
    if (strcmp(key, "x") == 0) {
        v[0] = value;
    } else if (strcmp(key, "y") == 0) {
        v[1] = value;
    } else if (strcmp(key, "z") == 0) {
        v[2] = value;
    }
    
    return 0;
}

static int vector3_tostring(lua_State* L) {
    float* v = (float*)luaL_checkudata(L, 1, VECTOR3_MT);
    lua_pushfstring(L, "Vector3(%f, %f, %f)", v[0], v[1], v[2]);
    return 1;
}

static int vector3_add(lua_State* L) {
    float x1, y1, z1, x2, y2, z2;
    checkVector3(L, 1, x1, y1, z1);
    checkVector3(L, 2, x2, y2, z2);
    
    pushVector3(L, x1 + x2, y1 + y2, z1 + z2);
    return 1;
}

static const luaL_Reg vector3_methods[] = {
    {"__index", vector3_index},
    {"__newindex", vector3_newindex},
    {"__tostring", vector3_tostring},
    {"__add", vector3_add},
    {NULL, NULL}
};

void registerVector3(lua_State* L) {
    // Create metatable
    luaL_newmetatable(L, VECTOR3_MT);
    
    // Set metatable methods
    luaL_setfuncs(L, vector3_methods, 0);
    
    // Pop metatable
    lua_pop(L, 1);
    
    // Register constructor
    lua_pushcfunction(L, vector3_new);
    lua_setglobal(L, "Vector3");
    
    spdlog::debug("Lua bindings: Vector3 registered");
}

// Transform registration
static const char* TRANSFORM_MT = "Transform";

static int transform_new(lua_State* L) {
    lua_newtable(L);
    
    // Create position
    pushVector3(L, 0, 0, 0);
    lua_setfield(L, -2, "position");
    
    // Create rotation
    pushVector3(L, 0, 0, 0);
    lua_setfield(L, -2, "rotation");
    
    // Create scale
    pushVector3(L, 1, 1, 1);
    lua_setfield(L, -2, "scale");
    
    // Set metatable
    luaL_getmetatable(L, TRANSFORM_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

static const luaL_Reg transform_methods[] = {
    {NULL, NULL}
};

void registerTransform(lua_State* L) {
    // Create metatable
    luaL_newmetatable(L, TRANSFORM_MT);
    
    // Set metatable methods
    luaL_setfuncs(L, transform_methods, 0);
    
    // Pop metatable
    lua_pop(L, 1);
    
    // Register constructor
    lua_pushcfunction(L, transform_new);
    lua_setglobal(L, "Transform");
    
    spdlog::debug("Lua bindings: Transform registered");
}

// Logging functions
static int lua_log_info(lua_State* L) {
    const char* message = luaL_checkstring(L, 1);
    spdlog::info("[Lua] {}", message);
    return 0;
}

static int lua_log_warn(lua_State* L) {
    const char* message = luaL_checkstring(L, 1);
    spdlog::warn("[Lua] {}", message);
    return 0;
}

static int lua_log_error(lua_State* L) {
    const char* message = luaL_checkstring(L, 1);
    spdlog::error("[Lua] {}", message);
    return 0;
}

void registerLogging(lua_State* L) {
    lua_pushcfunction(L, lua_log_info);
    lua_setglobal(L, "log_info");
    
    lua_pushcfunction(L, lua_log_warn);
    lua_setglobal(L, "log_warn");
    
    lua_pushcfunction(L, lua_log_error);
    lua_setglobal(L, "log_error");
    
    spdlog::debug("Lua bindings: Logging functions registered");
}

// Helper functions
void pushVector3(lua_State* L, float x, float y, float z) {
    float* v = (float*)lua_newuserdata(L, sizeof(float) * 3);
    v[0] = x;
    v[1] = y;
    v[2] = z;
    
    luaL_getmetatable(L, VECTOR3_MT);
    lua_setmetatable(L, -2);
}

bool checkVector3(lua_State* L, int index, float& x, float& y, float& z) {
    float* v = (float*)luaL_checkudata(L, index, VECTOR3_MT);
    if (v) {
        x = v[0];
        y = v[1];
        z = v[2];
        return true;
    }
    return false;
}