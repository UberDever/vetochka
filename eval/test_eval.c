#include <assert.h>
#include <stdbool.h>

#include "common.h"
#include "stb_ds.h"

void test_eval() {
  EvalState s = NULL;
  uint* nodes = NULL;
  stbds_arrput(nodes, node_new_app(1, 4));
  stbds_arrput(nodes, node_new_tree(1, 2));
  stbds_arrput(nodes, node_new_tree(node_new_invalid(), node_new_invalid()));
  stbds_arrput(nodes, node_new_tree(node_new_invalid(), node_new_invalid()));
  stbds_arrput(nodes, node_new_tree(node_new_invalid(), node_new_invalid()));

  eval_init(&s, 0, nodes, stbds_arrlen(nodes));
  eval_eval(s);

  eval_free(&s);
  stbds_arrfree(nodes);

  /* assert(s.error_code == 0); */
  /* assert(s.error[0] == '\0'); */
}


void test_memory() {
  Memory mem = NULL;
  uint* set_cells = NULL;
  const uint mem_size = 128;
  eval_memory_init(&mem, mem_size);
  for (uint i = 0; i < mem_size; ++i) {
    uint val = rand() % 5;
    stbds_arrput(set_cells, val);
    eval_memory_set_cell(mem, i, val);
  }
  for (uint i = 0; i < mem_size; ++i) {
    uint got = eval_memory_get_cell(mem, i);
    uint expected = set_cells[i];
    if (expected > 3) {
      assert(got == 0);
    } else {
      assert(got == expected);
    }
  }
  eval_memory_free(&mem);
  stbds_arrfree(set_cells);
}

int main() {
  test_eval();
  test_memory();
  return 0;
}
