#include "vetochka/public/bytecode/api.h"
#include "vetochka/public/lua-binding/impl.h"

#include <string.h>

#define CELLS_META "vetochka.cells"

// ---- nodes as Lua tables ----

void lb_push_node(lua_State* L, struct cells_node_t node) {
  lua_createtable(L, 0, 3);
  lua_pushinteger(L, node.header.type.value);
  lua_setfield(L, -2, "type");
  lua_pushinteger(L, (lua_Integer)node.header.encoded_size);
  lua_setfield(L, -2, "size");
  switch (node.header.type.value) {
  case CELLS_NODE_TYPE_REF:
    lua_pushinteger(L, node.as.ref);
    lua_setfield(L, -2, "ref");
    break;
  case CELLS_NODE_TYPE_VALUEF0:
  case CELLS_NODE_TYPE_VALUEF1:
  case CELLS_NODE_TYPE_VALUEF2:
    lua_pushinteger(L, node.as.nativef);
    lua_setfield(L, -2, "f");
    break;
  case CELLS_NODE_TYPE_VALUEV0:
  case CELLS_NODE_TYPE_VALUEV1:
  case CELLS_NODE_TYPE_VALUEV2:
    lua_pushlstring(L, (const char*)node.as.nativev.data, node.as.nativev.len);
    lua_setfield(L, -2, "v");
    break;
  default: break;
  }
}

// The payload of `v` points into the Lua string, which must stay on the stack while the node is used.
struct cells_node_t lb_check_node(lua_State* L, int index) {
  index = lua_absindex(L, index); // the fields below are pushed on the stack
  luaL_checktype(L, index, LUA_TTABLE);
  struct cells_node_t node;
  memset(&node, 0, sizeof node);
  lua_getfield(L, index, "type");
  node.header.type.value = (u8)luaL_checkinteger(L, -1);
  lua_getfield(L, index, "size");
  node.header.encoded_size = (size_t)luaL_checkinteger(L, -1);
  lua_getfield(L, index, "ref");
  lua_getfield(L, index, "f");
  lua_getfield(L, index, "v");
  if (lua_isinteger(L, -3)) node.as.ref = lua_tointeger(L, -3);
  if (lua_isinteger(L, -2)) node.as.nativef = lua_tointeger(L, -2);
  if (lua_isstring(L, -1)) {
    size_t len = 0;
    node.as.nativev.data = (byte*)lua_tolstring(L, -1, &len);
    node.as.nativev.len = len;
  }
  lua_pop(L, 5);
  return node;
}

static span_byte_t check_bytes(lua_State* L, int index) {
  size_t len = 0;
  const char* data = luaL_checklstring(L, index, &len);
  span_byte_t span = {(byte*)data, len};
  return span;
}

static cells_node_type_t check_type(lua_State* L, int index) {
  cells_node_type_t type = {(u8)luaL_checkinteger(L, index)};
  return type;
}

// ---- types and constructors ----

static int l_type_valid_raw(lua_State* L) {
  lua_pushboolean(L, cells_node_type_t_is_valid_raw((u8)luaL_checkinteger(L, 1)));
  return 1;
}
static int l_type_arity(lua_State* L) {
  lua_pushinteger(L, cells_node_type_get_arity(check_type(L, 1)));
  return 1;
}
static int l_type_with_arity(lua_State* L) {
  lua_pushinteger(L, cells_node_type_with_arity(check_type(L, 1), (u8)luaL_checkinteger(L, 2)).value);
  return 1;
}
static int l_type_encodable(lua_State* L) {
  lua_pushboolean(L, cells_node_type_is_encodable(check_type(L, 1)));
  return 1;
}

static int l_new_node(lua_State* L) { return lb_push_node(L, cells_new_node(check_type(L, 1))), 1; }
static int l_new_ref(lua_State* L) { return lb_push_node(L, cells_new_ref(luaL_checkinteger(L, 1))), 1; }
static int l_new_delta0(lua_State* L) { return lb_push_node(L, cells_new_delta0()), 1; }
static int l_new_delta1(lua_State* L) { return lb_push_node(L, cells_new_delta1()), 1; }
static int l_new_delta2(lua_State* L) { return lb_push_node(L, cells_new_delta2()), 1; }
static int l_new_value0f(lua_State* L) { return lb_push_node(L, cells_new_value0f(luaL_checkinteger(L, 1))), 1; }
static int l_new_value1f(lua_State* L) { return lb_push_node(L, cells_new_value1f(luaL_checkinteger(L, 1))), 1; }
static int l_new_value2f(lua_State* L) { return lb_push_node(L, cells_new_value2f(luaL_checkinteger(L, 1))), 1; }
static int l_new_value0v(lua_State* L) { return lb_push_node(L, cells_new_value0v(check_bytes(L, 1))), 1; }
static int l_new_value1v(lua_State* L) { return lb_push_node(L, cells_new_value1v(check_bytes(L, 1))), 1; }
static int l_new_value2v(lua_State* L) { return lb_push_node(L, cells_new_value2v(check_bytes(L, 1))), 1; }

// set_arity(node, arity) -> ok, the node with its new arity
static int l_set_arity(lua_State* L) {
  struct cells_node_t node = lb_check_node(L, 1);
  bool ok = cells_node_set_arity(&node, (u8)luaL_checkinteger(L, 2));
  lua_pushboolean(L, ok);
  lb_push_node(L, node);
  return 2;
}

// ---- the cells object ----

void lb_push_cells(lua_State* L, struct cells_t* cells) {
  struct cells_t** box = lua_newuserdatauv(L, sizeof *box, 0);
  *box = cells;
  luaL_setmetatable(L, CELLS_META);
}

struct cells_t* lb_check_cells(lua_State* L, int index) {
  return *(struct cells_t**)luaL_checkudata(L, index, CELLS_META);
}

static int push_error(lua_State* L, error_t err) {
  lua_pushnil(L);
  lua_pushinteger(L, err);
  return 2;
}

static int l_create(lua_State* L) {
  struct cells_t* cells = NULL;
  error_t err = cells_create(&cells, (size_t)luaL_checkinteger(L, 1));
  if (err < 0) return push_error(L, err);
  lb_push_cells(L, cells);
  return 1;
}

static int m_gc(lua_State* L) {
  struct cells_t** box = luaL_checkudata(L, 1, CELLS_META);
  cells_destroy(box);
  return 0;
}

static int m_alloc(lua_State* L) {
  size_t index = 0;
  error_t err = cells_alloc_chunk(lb_check_cells(L, 1), (size_t)luaL_checkinteger(L, 2), &index);
  if (err < 0) return push_error(L, err);
  lua_pushinteger(L, (lua_Integer)index);
  return 1;
}

static int m_write(lua_State* L) {
  error_t err = cells_write_node(lb_check_cells(L, 1), (size_t)luaL_checkinteger(L, 2), lb_check_node(L, 3));
  if (err < 0) return push_error(L, err);
  lua_pushboolean(L, 1);
  return 1;
}

static int m_header(lua_State* L) {
  struct cells_node_header_t header = cells_get_node_header(lb_check_cells(L, 1), (size_t)luaL_checkinteger(L, 2));
  lua_createtable(L, 0, 2);
  lua_pushinteger(L, header.type.value);
  lua_setfield(L, -2, "type");
  lua_pushinteger(L, (lua_Integer)header.encoded_size);
  lua_setfield(L, -2, "size");
  return 1;
}

// get(index) -> the node at index, decoded through its header
static int m_get(lua_State* L) {
  struct cells_t* cells = lb_check_cells(L, 1);
  size_t index = (size_t)luaL_checkinteger(L, 2);
  lb_push_node(L, cells_get_node(cells, index, cells_get_node_header(cells, index)));
  return 1;
}

static int m_free(lua_State* L) {
  error_t err = cells_node_free(lb_check_cells(L, 1), (size_t)luaL_checkinteger(L, 2), (size_t)luaL_checkinteger(L, 3));
  if (err < 0) return push_error(L, err);
  lua_pushboolean(L, 1);
  return 1;
}

static int m_span(lua_State* L) {
  span_cbyte_t span = cells_get_span(lb_check_cells(L, 1));
  lua_pushlstring(L, (const char*)span.data, span.len);
  return 1;
}

typedef error_t (*cells_walk_fn)(struct cells_t*, size_t*, struct cells_node_t*);

// Walk from index: -> the reached index and its node | nil, error
static int walk(lua_State* L, cells_walk_fn fn) {
  size_t index = (size_t)luaL_checkinteger(L, 2);
  struct cells_node_t node;
  memset(&node, 0, sizeof node);
  error_t err = fn(lb_check_cells(L, 1), &index, &node);
  if (err < 0) return push_error(L, err);
  lua_pushinteger(L, (lua_Integer)index);
  lb_push_node(L, node);
  return 2;
}
static int m_deref(lua_State* L) { return walk(L, cells_dereference_node); }
static int m_left(lua_State* L) { return walk(L, cells_get_left_node); }
static int m_right(lua_State* L) { return walk(L, cells_get_right_node); }

// A tree {node, [1] = left tree, [2] = right tree} into the builder; returns its builder index.
static size_t add_tree(lua_State* L, struct bytecode_tree_builder_t* b, int index) {
  luaL_checktype(L, index, LUA_TTABLE);
  lua_getfield(L, index, "node");
  struct cells_node_t node = lb_check_node(L, -1);
  size_t count = lua_rawlen(L, index);
  size_t built = 0;
  if (count == 0) {
    built = bytecode_new_node0(b, node);
  } else {
    lua_rawgeti(L, index, 1);
    size_t left = add_tree(L, b, lua_gettop(L));
    if (count == 1) {
      built = bytecode_new_node1(b, node, left);
    } else {
      lua_rawgeti(L, index, 2);
      size_t right = add_tree(L, b, lua_gettop(L));
      built = bytecode_new_node2(b, node, left, right);
      lua_pop(L, 1);
    }
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
  return built;
}

#define BUILDER_META "vetochka.builder"

static int builder_gc(lua_State* L) {
  struct bytecode_tree_builder_t** box = luaL_checkudata(L, 1, BUILDER_META);
  if (*box) bytecode_tree_builder_destroy(box);
  return 0;
}

// build(tree) -> the index of its root in these cells | nil, error
// The builder lives in a userdata, so a Lua error in a malformed tree cannot leak it.
static int m_build(lua_State* L) {
  struct cells_t* cells = lb_check_cells(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);
  struct bytecode_tree_builder_t** box = lua_newuserdatauv(L, sizeof *box, 0);
  *box = NULL;
  luaL_setmetatable(L, BUILDER_META);
  error_t err = bytecode_tree_builder_create(box);
  if (err < 0) return push_error(L, err);
  add_tree(L, *box, 2);
  size_t index = 0;
  err = bytecode_tree_builder_build(*box, cells, &index);
  bytecode_tree_builder_destroy(box);
  if (err < 0) return push_error(L, err);
  lua_pushinteger(L, (lua_Integer)index);
  return 1;
}

void lb_open_cells(lua_State* L) {
  static const luaL_Reg methods[] = {
      {"alloc", m_alloc}, {"write", m_write}, {"header", m_header}, {"get", m_get},
      {"free", m_free},   {"span", m_span},   {"deref", m_deref},   {"left", m_left},
      {"right", m_right}, {"build", m_build}, {NULL, NULL},
  };
  luaL_newmetatable(L, BUILDER_META);
  lua_pushcfunction(L, builder_gc);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);

  luaL_newmetatable(L, CELLS_META);
  lua_pushcfunction(L, m_gc);
  lua_setfield(L, -2, "__gc");
  lua_newtable(L);
  luaL_setfuncs(L, methods, 0);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  static const luaL_Reg functions[] = {
      {"create", l_create},
      {"type_valid_raw", l_type_valid_raw},
      {"type_arity", l_type_arity},
      {"type_with_arity", l_type_with_arity},
      {"type_encodable", l_type_encodable},
      {"new_node", l_new_node},
      {"new_ref", l_new_ref},
      {"new_delta0", l_new_delta0},
      {"new_delta1", l_new_delta1},
      {"new_delta2", l_new_delta2},
      {"new_value0f", l_new_value0f},
      {"new_value1f", l_new_value1f},
      {"new_value2f", l_new_value2f},
      {"new_value0v", l_new_value0v},
      {"new_value1v", l_new_value1v},
      {"new_value2v", l_new_value2v},
      {"set_arity", l_set_arity},
      {NULL, NULL},
  };
  lua_newtable(L);
  luaL_setfuncs(L, functions, 0);
  // cells.type: name -> raw value, from the same X-macro as the C enum
  lua_newtable(L);
#define LB_TYPE_FIELD(P, NAME, VALUE, STR) (lua_pushinteger(L, VALUE), lua_setfield(L, -2, #NAME));
  CELLS_NODE_TYPE_ITEMS(LB_TYPE_FIELD, _)
#undef LB_TYPE_FIELD
  lua_setfield(L, -2, "type");
  lua_setfield(L, -2, "cells");
}
