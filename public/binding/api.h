#ifndef VETOCHKA_BINDING_API_H
#define VETOCHKA_BINDING_API_H

#include "lua.h"
#include "vetochka/public/domain/api.h"

// The Lua module `vetochka`: require it after luaL_requiref(L, "vetochka", luaopen_vetochka, 0).
MUH_PUBLIC int luaopen_vetochka(lua_State* L);

#endif // VETOCHKA_BINDING_API_H
