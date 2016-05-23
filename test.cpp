#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <locale>
#include <cstdint>
#include "luna.h"

int func_a(int a, int b, int c)
{
    puts("func_a");
}

void func_b()
{
	puts("func_b");
}

void call_some_func(lua_State* L)
{
	const char* msg = nullptr;
	int len = 0;

	if (lua_call_file_function(L, "test.lua", "some_func", std::tie(msg, len), "hello", "world"))
		printf("msg=%s, len=%d\n", msg, len);

    lua_call_cleanup(L);
}

int main(int argc, char* argv[])
{
	lua_State* L = luaL_newstate();

	luaL_openlibs(L);

	lua_setup_env(L);

	lua_register_function(L, "fuck_a", func_a);
	lua_register_function(L, "fuck_b", func_b);

	call_some_func(L);

	lua_close(L);
	return 0;
}
