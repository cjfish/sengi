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

	if (lua_call_file_function(L, "test.lua", "some_func", ret_group(msg, len), arg_group("hello", "world")))
		printf("msg=%s, len=%d\n", msg, len);
}

int main(int argc, char* argv[])
{
	lua_State* L = lua_open();

	lua_register_function(L, "fuck_a", func_a);
	lua_register_function(L, "fuck_b", func_b);

	lua_close(L);
	return 0;
}
