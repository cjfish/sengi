#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <locale>
#include <cstdint>
#include "luna.h"

int sum(int a, int b)
{
    return a + b;
}

int main(int argc, char* argv[])
{
	lua_State* L = lua_open();

	lua_register_function(L, sum);

    int a, b, sum;
    lua_call_file_function(L, "test.lua", "test_sum", ret_group(a, b, sum), arg_group(2, 4));

    printf("%d + %d = %d\n", a, b, sum);

	lua_close(L);
	return 0;
}
