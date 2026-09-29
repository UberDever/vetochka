#include "vetochka/public/lua-binding/api.h"
#include "vetochka/public/lua-binding/impl.h"

int luaopen_vetochka(lua_State* L) {
  lua_newtable(L);
  lb_open_source(L);
  lb_open_cells(L);
  lb_open_reducer(L);
  return 1;
}
