#pragma once

#include <assert.h>
#include <string.h>
#include <cstdint>
#include <string>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include "lua.hpp"

typedef std::function<int(lua_State* L)> lua_function_wrapper;

template <typename T> T         lua_to_value(lua_State* L, int i)            {return T(0);}
template <> inline int32_t      lua_to_value<int32_t>(lua_State* L, int i)   { return (int32_t)lua_tointeger(L, i); }
template <> inline int64_t      lua_to_value<int64_t>(lua_State* L, int i)   { return (int64_t)lua_tointeger(L, i); }
template <> inline float        lua_to_value<float>(lua_State* L, int i)     { return (float)lua_tonumber(L, i); }
template <> inline double       lua_to_value<double>(lua_State* L, int i)    { return (double)lua_tonumber(L, i); }
template <> inline char*        lua_to_value<char*>(lua_State* L, int i)     { return (char*)lua_tostring(L, i); }
template <> inline const char*  lua_to_value<const char*>(lua_State* L, int i) { return lua_tostring(L, i); }
template<size_t... Integers, typename... var_types>
void lua_to_value_multi(lua_State* L, std::tuple<var_types&...>& vars, std::index_sequence<Integers...>&&)
{
    constexpr int ret_count = sizeof...(Integers);
    int _[] = { 0, (std::get<Integers>(vars) = lua_to_value<var_types>(L, (int)Integers - ret_count), 0)... };
}

inline int lua_push_value(lua_State* L, int32_t v)      { lua_pushinteger(L, v); return 0;}
inline int lua_push_value(lua_State* L, int64_t v)      { lua_pushinteger(L, v); return 0;}
inline int lua_push_value(lua_State* L, float v)        { lua_pushnumber(L, v); return 0;}
inline int lua_push_value(lua_State* L, double v)       { lua_pushnumber(L, v); return 0;}
inline int lua_push_value(lua_State* L, const char* v)  { lua_pushstring(L, v); return 0;}
template<size_t... Integers, typename... var_types>
void lua_push_value_multi(lua_State* L, std::tuple<var_types&...>& vars, std::index_sequence<Integers...>&&)
{
    int _[] = { 0, (lua_push_value(L, std::get<Integers>(vars)))... };
}


template<size_t... Integers, typename return_type, typename... arg_types>
return_type call_wrapper(lua_State* L, return_type(*func)(arg_types...), std::index_sequence<Integers...>&&)
{
    return (*func)(lua_to_value<arg_types>(L, Integers + 1)...);
}

template <typename return_type, typename... arg_types>
lua_function_wrapper create_function_wrapper(return_type(*func)(arg_types...))
{
    return [=](lua_State* L)
    {
        lua_push_value(L, call_wrapper(L, func, std::make_index_sequence<sizeof...(arg_types)>()));
        return 1;
    };
}

template <typename... arg_types>
lua_function_wrapper create_function_wrapper(void(*func)(arg_types...))
{
    return [=](lua_State* L)
    {
        call_wrapper(L, func, std::make_index_sequence<sizeof...(arg_types)>());
        return 0;
    };
}

template <>
inline lua_function_wrapper create_function_wrapper(int(*func)(lua_State* L))
{
    return func;
}

extern void lua_register_function_as(lua_State* L, const char* name, lua_function_wrapper func);
extern bool lua_get_file_function(lua_State* L, const char file_name[], const char function[]);
extern bool lua_get_table_function(lua_State* L, const char table[], const char function[]);
extern bool lua_call_function(lua_State* L, int arg_count, int ret_count);

template <typename T> 
void lua_register_function_as(lua_State* L, const char* name, T func)
{
    lua_register_function_as(L, name, create_function_wrapper(func));
}

template <typename... ret_types, typename... arg_types>
bool lua_call_function(lua_State* L, std::tuple<ret_types&...>& rets, std::tuple<arg_types&...>& args)
{
    constexpr int arg_count = sizeof...(arg_types);
    lua_push_value_multi(L, args, std::make_index_sequence<arg_count>());

    constexpr int ret_count = sizeof...(ret_types);
    if (!lua_call_function(L, arg_count, ret_count))
        return false;

    lua_to_value_multi(L, rets, std::make_index_sequence<ret_count>());
    return true;
}

