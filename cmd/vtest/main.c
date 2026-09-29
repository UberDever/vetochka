// vtest: runs a Lua script with the `vetochka` module available.
// Usage: vtest <script.lua> [args...]; the script sees its arguments in `arg`.

#include <stdio.h>

#include "lauxlib.h"
#include "lualib.h"
#include "vetochka/public/lua-binding/api.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <script.lua> [args...]\n", argv[0]);
    return 2;
  }
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);
  luaL_requiref(L, "vetochka", luaopen_vetochka, 0);
  lua_pop(L, 1);

  lua_createtable(L, argc - 2, 1);
  for (int i = 1; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i - 1);
  }
  lua_setglobal(L, "arg");

  int status = luaL_loadfile(L, argv[1]);
  if (status == LUA_OK) status = lua_pcall(L, 0, 1, 0);
  int code = 0;
  if (status != LUA_OK) {
    fprintf(stderr, "vtest: %s\n", lua_tostring(L, -1));
    code = 1;
  } else if (lua_isinteger(L, -1)) {
    code = (int)lua_tointeger(L, -1);
  }
  lua_close(L);
  return code;
}
