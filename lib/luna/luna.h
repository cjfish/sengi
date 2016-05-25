
/*
 * Author: trumanzhao(https://github.com/trumanzhao/luna)
 * Modified: brianzhang
 *
 * Example:
 *
 * --- test.c
 * #include "luna.h"
 *
 * int sum(int a, int b)
 * {
 *      return a + b;
 * }
 *
 * int main(int argc, char** argv)
 * {
 *      lua_State* L = lua_open();
 *      lua_export(L, sum);
 *
 *      int a, b, sum;
 *      lua_call_file_function(L, "test.lua", "test_sum", ret_group(a, b, sum), arg_group(3, 4));
 *      printf("%d + %d = %d\n", a, b, sum);
 *
 *      lua_close(L);
 *      return 0;
 * }
 *
 * --- test.lua
 * function onload()
 *      print(__FILE__ .. " loaded");
 * end
 *
 * function onreload()
 *      print(__FILE__ .. " reloaded");
 * end
 *
 * function test_sum(a, b)
 *      return a, b, sum(a, b);
 * end
 *
*/

#pragma once

#include "luna_wrapper.h"

/* Create lua VM */
lua_State* lua_open(std::function<void(const char*)>* error_func = nullptr);

/* Export C function to lua */
#define lua_export(L, func)    lua_register_cfunction(L, #func, func)

/* Load and reload lua script */
bool lua_load_script(lua_State* L, const char file_name[]);
void lua_reload_scripts(lua_State* L);

/* Call lua script function */
#define ret_group   std::tie
#define arg_group   std::forward_as_tuple

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

inline bool lua_call_file_function(lua_State* L, const char file_name[], const char function[]) {return lua_call_file_function(L, file_name, function, ret_group(), arg_group());}
inline bool lua_call_table_function(lua_State* L, const char table[], const char function[]) {return lua_call_table_function(L, table, function, ret_group(), arg_group());}
inline bool lua_call_global_function(lua_State* L, const char function[]) {return lua_call_global_function(L, function, ret_group(), arg_group());}

