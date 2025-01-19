#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

#define ERROR_BUF_SIZE 65536
static char g_error_buf[ERROR_BUF_SIZE] = {};

enum error_t { error_max_iters = 1 };

struct EvalState {
  uint root;
  uint *nodes;
  uint nodes_size;

  i8 error_code;
  char *error;
};

void reset(struct EvalState *state) {
  state->root = 0;
  state->nodes = NULL;
  state->nodes_size = 0;
  state->error_code = 0;
  state->error = g_error_buf;
}

#define MAX_ITERS (uint)65535

uint eval(struct EvalState *state) {
  uint i = 0;
  while (1) {
    if (i >= MAX_ITERS) {
      state->error_code = error_max_iters;
      sprintf(g_error_buf, "%sMaximum iteration count %zu exceeded\n",
              g_error_buf, MAX_ITERS);
      return 0;
    }

    const uint root_i = state->root;
    const uint root_node = state->nodes[root_i];
    switch (node_tag(root_node)) {
    case NODE_APP: {
      const uint anchor_i = node_lhs(root_node);
      assert(anchor_i != node_new_invalid());
      const Node anchor = state->nodes[root_i + anchor_i];
      assert(node_tag(anchor) == node_tag_tree());

      const uint program_i = node_lhs(anchor);
      assert(program_i != node_new_invalid());
      const Node program = state->nodes[anchor_i + program_i];
      // NOTE: consider program evaluation strategy

      const uint program_lhs = node_lhs(program);
      const uint program_rhs = node_rhs(program);

      // ^ ^ a b => a
      if (program_lhs == node_new_invalid() &&
          program_rhs == node_new_invalid()) {
        assert(node_tag(program) == node_tag_tree());
        const uint first_i = node_rhs(anchor);
        assert(first_i != node_new_invalid());
        state->root = root_i + anchor_i + first_i;
        break;
      }

      // ^ (^ a) b c => a c (b c)
      if (program_rhs == node_new_invalid()) {
        // NOTE: we need resize here
        break;
      }

      // ^ (^ a b) c d => d a b
      /* const uint predicate_i = node_rhs(root_node); */
      /* assert(program_i != node_new_invalid()); */
      /* const Node predicate = state->nodes[root_i + predicate_i]; */
      break;
    }
    case NODE_TREE: {
      return 0;
    }
    default: {
      assert(0);
    }
    }

    i++;
  }
  return 1;
}
