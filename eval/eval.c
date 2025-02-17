#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "stb_ds.h"

#define ERROR_BUF_SIZE 65536
static char g_error_buf[ERROR_BUF_SIZE] = {};

enum error_t { error_max_iters = 1 };

struct EvalState {
  uint root;
  uint* nodes;
  uint nodes_size;

  i8 error_code;
  char* error;
};

void reset(struct EvalState* state) {
  state->root = 0;
  state->nodes = NULL;
  state->nodes_size = 0;
  state->error_code = 0;
  state->error = g_error_buf;
}

void init(struct EvalState* state, uint root, const uint* nodes, uint nodes_size) {
  reset(state);
  state->root = root;
  state->nodes_size = nodes_size;
  for (uint i = 0; i < nodes_size; ++i) {
    stbds_arrput(state->nodes, nodes[i]);
  }
}

struct NodeWithPos {
  uint node;
  sint shift;
  sint pos;
};

static inline struct NodeWithPos fetch_node(const uint* nodes, uint i) {
  const uint n = nodes[i];
  return (struct NodeWithPos){.node = n, .shift = i, .pos = i};
}

static inline struct NodeWithPos fetch_lhs(uint* nodes, struct NodeWithPos node) {
  const uint shift = node_lhs(node.node);
  if (shift == node_new_invalid()) {
    return (struct NodeWithPos){node_new_invalid(), node_new_invalid(), node_new_invalid()};
  }
  const uint i = node.pos + shift;
  return (struct NodeWithPos){.node = nodes[i], .shift = shift, .pos = i};
}

static inline struct NodeWithPos fetch_rhs(uint* nodes, struct NodeWithPos node) {
  const uint shift = node_rhs(node.node);
  if (shift == node_new_invalid()) {
    return (struct NodeWithPos){node_new_invalid(), node_new_invalid(), node_new_invalid()};
  }
  const uint i = node.pos + shift;
  return (struct NodeWithPos){.node = nodes[i], .shift = shift, .pos = i};
}

uint step(struct EvalState* state) {
  const struct NodeWithPos root = fetch_node(state->nodes, state->root);
  if (node_tag(root.node) == NODE_APP) {
    const struct NodeWithPos delta = fetch_lhs(state->nodes, root);
    const struct NodeWithPos arg = fetch_rhs(state->nodes, root);
    // NOTE: Comma in following rules used as infix application operator
    // 1. ^, z -> ^ z
    if (node_tag(delta.node) == NODE_TREE &&                //
        node_lhs(delta.node) == (sint)node_new_invalid() && //
        node_rhs(delta.node) == (sint)node_new_invalid()) {
      state->nodes[delta.pos] = node_new_tree(arg.pos - delta.pos, node_new_invalid());
      state->root = delta.pos;
      return 0;
    }
    // 2. ^ y, z -> ^ y z
    if (node_tag(delta.node) == NODE_TREE && //
        node_rhs(delta.node) == (sint)node_new_invalid()) {
      state->nodes[delta.pos] = node_new_tree(node_lhs(delta.node), arg.pos - delta.pos);
      state->root = delta.pos;
      return 0;
    }
    // 3. ^ ^ y, z -> y
    const struct NodeWithPos delta_left = fetch_lhs(state->nodes, delta);
    const struct NodeWithPos delta_right = fetch_rhs(state->nodes, delta);
    if (node_tag(delta.node) == NODE_TREE &&                     //
        node_tag(delta_left.node) == NODE_TREE &&                //
        node_lhs(delta_left.node) == (sint)node_new_invalid() && //
        node_rhs(delta_left.node) == (sint)node_new_invalid()    //
    ) {
      state->root = delta_right.pos;
      return 0;
    }
    // 4. ^ (^ x) y, z -> ($ ($ y z) ($ x z))
    if (node_tag(delta.node) == NODE_TREE &&                  //
        node_tag(delta_left.node) == NODE_TREE &&             //
        node_rhs(delta_left.node) == (sint)node_new_invalid() //
    ) {
      const struct NodeWithPos x = fetch_lhs(state->nodes, delta_left);
      const struct NodeWithPos y = delta_right;
      const struct NodeWithPos z = arg;
      sint app_pos = stbds_arrlen(state->nodes);
      stbds_arrput(state->nodes, node_new_app(app_pos - y.pos, app_pos - z.pos));

      // TODO: possibly fix
      state->root = app_pos;
      uint error = step(state);
      if (error) {
        return error;
      }

      const sint app_lhs_pos = app_pos;
      app_pos = stbds_arrlen(state->nodes);
      stbds_arrput(state->nodes, node_new_app(app_pos - x.pos, app_pos - z.pos));

      state->root = app_pos;
      error = step(state);
      if (error) {
        return error;
      }

      const sint app_rhs_pos = app_pos;
      app_pos = stbds_arrlen(state->nodes);
      stbds_arrput(state->nodes, node_new_app(app_pos - app_lhs_pos, app_pos - app_rhs_pos));
      return 0;
    }

    // NOTE: The following is the triage-calculus, baking the rules of triage into reduction itself
    // 5 (3a). ^ (^ a b) c ^ -> a
    if (node_tag(delta.node) == NODE_TREE &&   //
        node_tag(delta_left.node) == NODE_TREE && //
        node_tag(arg) == NODE_TREE // what if NODE_APP?
    ) {
        const struct NodeWithPos w = fetch_lhs(state->nodes, delta_left);
        const struct NodeWithPos x = fetch_rhs(state->nodes, delta_left);
        const struct NodeWithPos z = arg;

    }
  }
  assert(node_tag(root.node) == NODE_TREE);
  return 0;
}

#define MAX_ITERS (uint)65535

uint eval(struct EvalState* state) {
  uint i = 0;
  while (1) {
    if (i >= MAX_ITERS) {
      state->error_code = error_max_iters;
      sprintf(g_error_buf, "%sMaximum iteration count %zu exceeded\n", g_error_buf, MAX_ITERS);
      return 0;
    }

    if (step(state)) {
      return 1;
    }

    i++;
  }
  return 1;
}
