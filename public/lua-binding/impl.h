#ifndef VETOCHKA_LUA_BINDING_IMPL_H
#define VETOCHKA_LUA_BINDING_IMPL_H

#include "lauxlib.h"
#include "vetochka/public/cells/api.h"

// The binding mirrors the C API one to one; it is for tests, and transient.
// Nodes are Lua tables {type, size, ref | f | v}; `v` is the byte payload as a string.

void lb_push_node(lua_State* L, struct cells_node_t node);
struct cells_node_t lb_check_node(lua_State* L, int index);

// Push a new cells object owning `cells`.
void lb_push_cells(lua_State* L, struct cells_t* cells);
struct cells_t* lb_check_cells(lua_State* L, int index);

// Add the module's functions to the table on top of the stack.
void lb_open_source(lua_State* L);
void lb_open_cells(lua_State* L);
void lb_open_reducer(lua_State* L);

#endif // VETOCHKA_LUA_BINDING_IMPL_H
