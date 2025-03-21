#include <assert.h>
#include <stdbool.h>

#include "common.h"
#include "stb_ds.h"

void test_eval() {
  struct EvalState s = {};
  uint *nodes = NULL;
  stbds_arrput(nodes, node_new_app(1, 4));
  stbds_arrput(nodes, node_new_tree(1, 2));
  stbds_arrput(nodes, node_new_tree(node_new_invalid(), node_new_invalid()));
  stbds_arrput(nodes, node_new_tree(node_new_invalid(), node_new_invalid()));
  stbds_arrput(nodes, node_new_tree(node_new_invalid(), node_new_invalid()));
  
  init(&s, 0, nodes, stbds_arrlen(nodes));
  eval(&s);
  
  reset(&s);
  stbds_arrfree(nodes);

  assert(s.error_code == 0);
  assert(s.error[0] == '\0');
}

int main() {
  test_eval();
  return 0;
}
