#include "vetochka/public/allocator/api.h"
#include "vetochka/public/bytecode/api.h"
#include "vetochka/public/lua-binding/impl.h"
#include "vetochka/public/source/api.h"

typedef error_t (*source_format_fn)(span_cbyte_t, const struct source_tree_t*, struct da_byte_t*);

// Parse the text argument and push `format` of its tree, or nil and a message.
static int push_formatted(lua_State* L, source_format_fn format) {
  size_t len = 0;
  const char* text = luaL_checklstring(L, 1, &len);
  span_cbyte_t span = {(const byte*)text, len};
  struct allocator_t allocator = allocator_libc();

  struct source_tree_t* tree = NULL;
  if (source_ast_create(&tree, span, allocator) < 0) {
    source_tree_free(&tree);
    lua_pushnil(L);
    lua_pushliteral(L, "parse error");
    return 2;
  }
  struct da_byte_t out;
  error_t err = domain_da_byte_init(&out, &allocator);
  if (err >= 0) err = format(span, tree, &out);
  if (err >= 0) {
    span_cbyte_t result = domain_da_byte_get_span(&out);
    lua_pushlstring(L, (const char*)result.data, result.len);
  }
  domain_da_byte_free(&out);
  source_tree_free(&tree);
  if (err < 0) {
    lua_pushnil(L);
    lua_pushfstring(L, "format error %d", (int)err);
    return 2;
  }
  return 1;
}

// vetochka.parse(text) -> the syntax tree as an s-expression | nil, message
static int l_parse(lua_State* L) { return push_formatted(L, source_tree_format_sexpr); }

// vetochka.format(text) -> the text in canonical form | nil, message
static int l_format(lua_State* L) { return push_formatted(L, source_tree_format_canonical); }

// vetochka.encode(text [, {source_meta = bool}]) -> cells, the root index | nil, message
static int l_encode(lua_State* L) {
  size_t len = 0;
  const char* text = luaL_checklstring(L, 1, &len);
  struct bytecode_source_options_t options = {false};
  if (lua_istable(L, 2)) {
    lua_getfield(L, 2, "source_meta");
    options.source_meta = lua_toboolean(L, -1);
    lua_pop(L, 1);
  }
  span_cbyte_t span = {(const byte*)text, len};
  struct source_tree_t* tree = NULL;
  if (source_ast_create(&tree, span, allocator_libc()) < 0) {
    source_tree_free(&tree);
    lua_pushnil(L);
    lua_pushliteral(L, "parse error");
    return 2;
  }
  struct cells_t* cells = NULL;
  error_t err = cells_create(&cells, 8192);
  size_t root = 0;
  if (err >= 0) err = bytecode_source_encode(span, tree, options, cells, &root);
  source_tree_free(&tree);
  if (err < 0) {
    cells_destroy(&cells);
    lua_pushnil(L);
    lua_pushfstring(L, "encode error %d", (int)err);
    return 2;
  }
  lb_push_cells(L, cells);
  lua_pushinteger(L, (lua_Integer)root);
  return 2;
}

void lb_open_source(lua_State* L) {
  static const luaL_Reg functions[] = {
      {"parse", l_parse},
      {"format", l_format},
      {"encode", l_encode},
      {NULL, NULL},
  };
  luaL_setfuncs(L, functions, 0);
}
