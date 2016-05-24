#pragma once

#include "luna_wrapper.h"

/* Setup runtime environment */
lua_State* lua_open(std::function<void(const char*)>* error_func = nullptr);

/* Load and reload lua script */
bool lua_load_string(lua_State* L, const char env[], const char code[], int code_len = -1);
bool lua_load_script(lua_State* L, const char file_name[]);
void lua_reload_update(lua_State* L);

/* Export C function to lua script */
template <typename T> 
void lua_register_function(lua_State* L, const char* name, T func)
{
    lua_register_function(L, name, create_function_wrapper(func));
}

/* Call lua script function */
#define arg_group   std::tie
#define ret_group   std::tie

template <typename... ret_types, typename... arg_types>
bool lua_call_file_function(lua_State* L, const char file_name[], const char function[], 
                            std::tuple<ret_types&...>&& rets, std::tuple<arg_types&...>&& args)
{
    lua_settop(L, 0);

    if (!lua_get_file_function(L, file_name, function))
        return false;

    return lua_call_function(L, rets, args);
}

template <typename... ret_types, typename... arg_types>
bool lua_call_table_function(lua_State* L, const char table[], const char function[], 
                            std::tuple<ret_types&...>&& rets, std::tuple<arg_types&...>&& args)
{
    lua_settop(L, 0);

    if (!lua_get_table_function(L, table, function))
        return false;

    return lua_call_function(L, rets, args);
}

template <typename... ret_types, typename... arg_types>
bool lua_call_global_function(lua_State* L, const char function[],
                            std::tuple<ret_types&...>&& rets, std::tuple<arg_types&...>&& args)
{
    lua_settop(L, 0);

    if (lua_getglobal(L, function) != LUA_OK || !lua_isfunction(L, -1))
        return false;

    return lua_call_function(L, rets, args);
}

inline bool lua_call_file_function(lua_State* L, const char file_name[], const char function[]) { return lua_call_file_function(L, file_name, function, ret_group(), arg_group()); }
inline bool lua_call_table_function(lua_State* L, const char table[], const char function[]) { return lua_call_table_function(L, table, function, ret_group(), arg_group()); }
inline bool lua_call_global_function(lua_State* L, const char function[]) { return lua_call_global_function(L, function, ret_group(), arg_group()); }

