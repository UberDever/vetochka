#include "vetochka/public/lua-binding/impl.h"
#include "vetochka/public/reducer/api.h"

#define REDUCER_META "vetochka.reducer"

static struct reducer_t* check_reducer(lua_State* L) {
  return *(struct reducer_t**)luaL_checkudata(L, 1, REDUCER_META);
}

// cells:reducer() -> a reducer over these cells; it keeps them alive
static int l_reducer(lua_State* L) {
  struct cells_t* cells = lb_check_cells(L, 1);
  struct reducer_t** box = lua_newuserdatauv(L, sizeof *box, 1);
  *box = NULL;
  luaL_setmetatable(L, REDUCER_META);
  lua_pushvalue(L, 1);
  lua_setiuservalue(L, -2, 1);
  error_t err = reducer_create(box, cells);
  if (err < 0) {
    lua_pushnil(L);
    lua_pushinteger(L, err);
    return 2;
  }
  return 1;
}

static int m_gc(lua_State* L) {
  struct reducer_t** box = luaL_checkudata(L, 1, REDUCER_META);
  if (*box) reducer_free(box);
  return 0;
}

// push(index | "apply")
static int m_push(lua_State* L) {
  size_t index = lua_type(L, 2) == LUA_TSTRING && lua_rawlen(L, 2) == 5
      ? REDUCER_APPLY_TOKEN
      : (size_t)luaL_checkinteger(L, 2);
  reducer_push_to_stack(check_reducer(L), index);
  return 0;
}

// step() -> the step's code: REDUCER_DONE when done, negative on error
static int m_step(lua_State* L) {
  lua_pushinteger(L, reducer_step(check_reducer(L)));
  return 1;
}

static int m_result(lua_State* L) {
  lua_pushinteger(L, (lua_Integer)reducer_get_result(check_reducer(L)));
  return 1;
}

static int m_has_result(lua_State* L) {
  lua_pushboolean(L, reducer_has_result(check_reducer(L)));
  return 1;
}

static int m_reset(lua_State* L) {
  reducer_reset(check_reducer(L));
  return 0;
}

static int m_error(lua_State* L) {
  const char* err = reducer_get_error(check_reducer(L));
  if (err) lua_pushstring(L, err);
  else lua_pushnil(L);
  return 1;
}

void lb_open_reducer(lua_State* L) {
  static const luaL_Reg methods[] = {
      {"push", m_push},   {"step", m_step},   {"result", m_result}, {"has_result", m_has_result},
      {"reset", m_reset}, {"error", m_error}, {NULL, NULL},
  };
  luaL_newmetatable(L, REDUCER_META);
  lua_pushcfunction(L, m_gc);
  lua_setfield(L, -2, "__gc");
  lua_newtable(L);
  luaL_setfuncs(L, methods, 0);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  // cells objects get :reducer()
  luaL_getmetatable(L, "vetochka.cells");
  lua_getfield(L, -1, "__index");
  lua_pushcfunction(L, l_reducer);
  lua_setfield(L, -2, "reducer");
  lua_pop(L, 2);

  lua_getfield(L, -1, "cells");
  lua_pushinteger(L, REDUCER_DONE);
  lua_setfield(L, -2, "REDUCER_DONE");
  lua_pop(L, 1);
}
