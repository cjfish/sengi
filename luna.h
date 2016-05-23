#pragma once

#include "luna_wrapper.h"

/* Setup runtime environment */
void lua_setup_env(lua_State*L, std::function<void(const char*)>* error_func = nullptr);

/* Load and reload lua script */
bool lua_load_string(lua_State* L, const char env[], const char code[], int code_len = -1);
bool lua_load_script(lua_State* L, const char file_name[]);
void lua_reload_update(lua_State* L);

/* Export C function to lua script */
void lua_register_function(lua_State* L, const char* name, lua_function_wrapper func);
template <typename T> void lua_register_function(lua_State* L, const char* name, T func)
{
    lua_register_function(L, name, create_function_wrapper(func));
}

/* Call lua script function */
template <typename... ret_types, typename... arg_types>
bool lua_call_file_function(lua_State* L, const char file_name[], const char function[], std::tuple<ret_types&...>&& rets, arg_types... args)
{
    lua_call_prepare(L);

    if (!lua_get_file_function(L, file_name, function))
        return false;

    int _0[] = { 0, (lua_pushvalue(L, args), 0)... };
    constexpr int ret_count = sizeof...(ret_types);
    if (!lua_call_function(L, sizeof...(arg_types), ret_count))
        return false;

    lua_tovalue_mutil(L, rets, std::make_index_sequence<ret_count>());
    return true;
}

template <typename... ret_types, typename... arg_types>
bool lua_call_table_function(lua_State* L, const char table[], const char function[], std::tuple<ret_types&...>&& rets, arg_types... args)
{
    lua_call_prepare(L);

    if (!lua_get_table_function(L, table, function))
        return false;

    int _0[] = { 0, (lua_pushvalue(L, args), 0)... };
    constexpr int ret_count = sizeof...(ret_types);
    if (!lua_call_function(L, sizeof...(arg_types), ret_count))
        return false;

    lua_tovalue_mutil(L, rets, std::make_index_sequence<ret_count>());
    return true;
}

template <typename... ret_types, typename... arg_types>
bool lua_call_global_function(lua_State* L, const char function[], std::tuple<ret_types&...>&& rets, arg_types... args)
{
    lua_call_prepare(L);

    if (lua_getglobal(L, function) != LUA_OK || !lua_isfunction(L, -1))
        return false;

    int _0[] = { 0, (lua_pushvalue(L, args), 0)... };
    constexpr int ret_count = sizeof...(ret_types);
    if (!lua_call_function(L, sizeof...(arg_types), ret_count))
        return false;

    lua_tovalue_mutil(L, rets, std::make_index_sequence<ret_count>());
    return true;
}

inline bool lua_call_file_function(lua_State* L, const char file_name[], const char function[]) { return lua_call_file_function(L, file_name, function, std::tie()); }
inline bool lua_call_table_function(lua_State* L, const char table[], const char function[]) { return lua_call_table_function(L, table, function, std::tie()); }
inline bool lua_call_global_function(lua_State* L, const char function[]) { return lua_call_global_function(L, function, std::tie()); }

/* Cleanup lua stack after lua_call_xxx_function */
void lua_call_cleanup(lua_State* L);

