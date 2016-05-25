#include <sys/stat.h>
#include <sys/types.h>
#ifdef __linux
#include <dirent.h>
#endif
#include <map>
#include <string>
#include <algorithm>
#include <cstdio>
#include "luna.h"

#define LUNA_FILE_ENV_METATABLE     "__luna_file_env_meta__"
#define LUNA_FILE_ENV_PREFIX        "__luna_file:"
#define LUNA_RUNTIME_METATABLE      "__luna_runtime_meta__"
#define LUNA_RUNTIME_TABLE          "__luna_runtime__"

struct luna_runtime_t
{
    std::map<std::string, time_t> files;
    std::map<std::string, lua_cfunction_wrapper*> funcs;
    std::function<void(const char*)> error_func = [](const char* err) { puts(err); };
};

static char* skip_utf8_bom(char* text, size_t len)
{
    if (len >= 3 && text[0] == (char)0xEF && text[1] == (char)0xBB && text[2] == (char)0xBF)
        return text + 3;
    return text;
}

static bool get_file_time(time_t* mtime, const char file_name[])
{
    struct stat file_info;
    int ret = stat(file_name, &file_info);
    if (ret != 0)
        return false;
    *mtime = file_info.st_mtime;
    return true;
}

static bool get_file_size(size_t* size, const char file_name[])
{
    struct stat info;
    int ret = stat(file_name, &info);
    if (ret != 0)
        return false;
    *size = (size_t)info.st_size;
    return true;
}

static bool get_file_data(char* buffer, size_t size, const char file_name[])
{
    FILE* file = fopen(file_name, "rb");
    if (file == nullptr)
        return false;
    size_t rcount = fread(buffer, size, 1, file);
    fclose(file);
    return (rcount == 1);
}

static luna_runtime_t* get_luna_runtime(lua_State* L)
{
    lua_getglobal(L, LUNA_RUNTIME_TABLE);
    auto user_data = (luna_runtime_t**)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return user_data ? *user_data : nullptr;
}

static void print_error(lua_State* L, const char* text)
{
    auto runtime = get_luna_runtime(L);
    runtime->error_func(text);
}

static int file_env_index(lua_State* L)
{
    const char* key = lua_tostring(L, 2);
    if (key != nullptr)
    {
        lua_getglobal(L, key);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

static int lua_import(lua_State* L)
{
    int top = lua_gettop(L);
    const char* file_name = nullptr;
    std::string env_name = LUNA_FILE_ENV_PREFIX;

    if (top != 1 || !lua_isstring(L, 1))
    {
        lua_pushnil(L);
        return 1;
    }

    file_name = lua_tostring(L, 1);
    env_name += file_name;

    lua_getglobal(L, env_name.c_str());
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        lua_load_script(L, file_name);
        lua_getglobal(L, env_name.c_str());
    }
    return 1;
}

static int luna_runtime_gc(lua_State* L)
{
    auto user_data = (luna_runtime_t**)lua_touserdata(L, 1);
    auto runtime = *user_data;
    for (auto one : runtime->funcs)
    {
        delete one.second;
    }
    delete runtime;
    *user_data = nullptr;
    return 0;
}

lua_State* lua_open(std::function<void(const char*)>* error_func)
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

    auto runtime = get_luna_runtime(L);
    if (runtime != nullptr)
    {
        return L;
    }

    runtime = new luna_runtime_t();

    if (error_func)
    {
        runtime->error_func = *error_func;
    }

    auto user_data = (luna_runtime_t**)lua_newuserdata(L, sizeof(runtime));
    *user_data = runtime;
    lua_pushvalue(L, -1);
    lua_setglobal(L, LUNA_RUNTIME_TABLE);

    luaL_newmetatable(L, LUNA_RUNTIME_METATABLE);
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, luna_runtime_gc);
    lua_settable(L, -3);
    lua_setmetatable(L, -2);
    lua_pop(L, 1);

    luaL_newmetatable(L, LUNA_FILE_ENV_METATABLE);
    lua_pushstring(L, "__index");
    lua_pushcfunction(L, file_env_index);
    lua_settable(L, -3);
    lua_pop(L, 1);

    lua_register(L, "import", lua_import);

    return L;
}

static int Lua_run_cfunction_wrapper(lua_State* L)
{
    lua_cfunction_wrapper* func_ptr = (lua_cfunction_wrapper*)lua_touserdata(L, lua_upvalueindex(1));
    return (*func_ptr)(L);
}

void lua_register_cfunction(lua_State* L, const char* name, lua_cfunction_wrapper func)
{
    auto runtime = get_luna_runtime(L);
    lua_cfunction_wrapper* func_ptr = nullptr;
    auto it = runtime->funcs.find(name);
    if (it != runtime->funcs.end())
    {
        func_ptr = it->second;
        *func_ptr = func;
        if (!func)
        {
            lua_pushnil(L);
            lua_setglobal(L, name);
        }
        return;
    }

    func_ptr = new lua_cfunction_wrapper(func);
    runtime->funcs[name] = func_ptr;
    lua_pushlightuserdata(L, func_ptr);
    lua_pushcclosure(L, Lua_run_cfunction_wrapper, 1);
    lua_setglobal(L, name);
}

static bool lua_load_script_string(lua_State* L, const char env[], const char code[], int code_len)
{
    bool result = false;
    int top = lua_gettop(L);

    if (code_len == -1)
    {
        code_len = (int)strlen(code);
    }

    if (luaL_loadbuffer(L, code, code_len, env))
    {
        print_error(L, lua_tostring(L, -1));
        goto exit0;
    }

    lua_getglobal(L, env);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);

        // file env table
        lua_newtable(L);

        luaL_getmetatable(L, LUNA_FILE_ENV_METATABLE);
        lua_setmetatable(L, -2);

        lua_pushvalue(L, -1);
        lua_setglobal(L, env);
    }
    lua_setupvalue(L, -2, 1);

    if (lua_pcall(L, 0, 0, 0))
    {
        print_error(L, lua_tostring(L, -1));
        goto exit0;
    }

    result = true;
exit0:
    lua_settop(L, top);
    return result;
}

bool lua_load_script(lua_State* L, const char file_name[])
{
    bool result = false;
    auto runtime = get_luna_runtime(L);
    std::string file_path = file_name;
    std::string env_name = LUNA_FILE_ENV_PREFIX;
    time_t file_time = 0;
    size_t file_size = 0;
    char* buffer = nullptr;
    char* code = nullptr;

    env_name += file_path;

    if (!get_file_time(&file_time, file_name))
        goto exit0;

    if (!get_file_size(&file_size, file_name))
        goto exit0;

    buffer = new char[file_size];
    if (buffer == nullptr)
        goto exit0;

    if (!get_file_data(buffer, file_size, file_name))
        goto exit0;

    code = skip_utf8_bom(buffer, file_size);
    if (lua_load_script_string(L, env_name.c_str(), code, (int)(buffer + file_size - code)))
    {
        runtime->files[file_path] = file_time;
        result = true;
    }
exit0:
    delete[] buffer;
    return result;
}

void lua_reload_scripts(lua_State* L)
{
    auto runtime = get_luna_runtime(L);
    for (auto& one : runtime->files)
    {
        const char* file_name = one.first.c_str();
        time_t new_time = 0;
        if (get_file_time(&new_time, file_name))
        {
            if (new_time != one.second)
            {
                lua_load_script(L, file_name);
            }
        }
    }
}

bool lua_get_file_function(lua_State* L, const char file_name[], const char function[])
{
    bool result = false;
    int top = lua_gettop(L);
    std::string env_name = LUNA_FILE_ENV_PREFIX;

    env_name += file_name;
    lua_getglobal(L, env_name.c_str());

    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        if (!lua_load_script(L, file_name))
            goto Exit0;

        lua_getglobal(L, env_name.c_str());
    }
    lua_getfield(L, -1, function);
    lua_remove(L, -2);
    result = lua_isfunction(L, -1);
Exit0:
    if (!result)
    {
        lua_settop(L, top);
    }
    return result;
}

bool lua_get_table_function(lua_State* L, const char table[], const char function[])
{
    lua_getglobal(L, table);
    lua_getfield(L, -1, function);
    lua_remove(L, -2);
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }
    return true;
}

bool lua_call_function(lua_State* L, int arg_count, int ret_count)
{
    int func_idx = lua_gettop(L) - arg_count;
    if (func_idx <= 0 || !lua_isfunction(L, func_idx))
    {
        print_error(L, "call invalid function !");
        return false;
    }

    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_remove(L, -2); // remove 'debug'

    lua_insert(L, func_idx);
    if (lua_pcall(L, arg_count, ret_count, func_idx))
    {
        print_error(L, lua_tostring(L, -1));
        return false;
    }
    lua_remove(L, -ret_count - 1); // remove 'traceback'
    return true;
}

