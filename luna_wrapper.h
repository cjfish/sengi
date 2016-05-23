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

template <typename T> T         lua_tovalue(lua_State* L, int i)            {return T(0);}
template <> inline int32_t      lua_tovalue<int32_t>(lua_State* L, int i)   { return (int32_t)lua_tointeger(L, i); }
template <> inline int64_t      lua_tovalue<int64_t>(lua_State* L, int i)   { return (int64_t)lua_tointeger(L, i); }
template <> inline float        lua_tovalue<float>(lua_State* L, int i)     { return (float)lua_tonumber(L, i); }
template <> inline double       lua_tovalue<double>(lua_State* L, int i)    { return (double)lua_tonumber(L, i); }
template <> inline const char*  lua_tovalue<const char*>(lua_State* L, int i) { return lua_tostring(L, i); }
template<size_t... Integers, typename... var_types>
void lua_tovalue_mutil(lua_State* L, std::tuple<var_types&...>& vars, std::index_sequence<Integers...>&&)
{
    constexpr int ret_count = sizeof...(Integers);
    int _[] = { 0, (std::get<Integers>(vars) = lua_tovalue<var_types>(L, (int)Integers - ret_count), 0)... };
}

inline void lua_pushvalue(lua_State* L, int32_t v)      { lua_pushinteger(L, v); }
inline void lua_pushvalue(lua_State* L, int64_t v)      { lua_pushinteger(L, v); }
inline void lua_pushvalue(lua_State* L, float v)        { lua_pushnumber(L, v); }
inline void lua_pushvalue(lua_State* L, double v)       { lua_pushnumber(L, v); }
inline void lua_pushvalue(lua_State* L, const char* v)  { lua_pushstring(L, v); }

template<size_t... Integers, typename return_type, typename... arg_types>
return_type call_wrapper(lua_State* L, return_type(*func)(arg_types...), std::index_sequence<Integers...>&&)
{
    return (*func)(lua_tovalue<arg_types>(L, Integers + 1)...);
}

template <typename return_type, typename... arg_types>
lua_function_wrapper create_function_wrapper(return_type(*func)(arg_types...))
{
    return [=](lua_State* L)
    {
        lua_pushvalue(L, call_wrapper(L, func, std::make_index_sequence<sizeof...(arg_types)>()));
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

extern bool lua_get_file_function(lua_State* L, const char file_name[], const char function[]);
extern bool lua_get_table_function(lua_State* L, const char table[], const char function[]);
extern void lua_call_prepare(lua_State* L);
extern bool lua_call_function(lua_State* L, int arg_count, int ret_count);

