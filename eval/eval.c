#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "common.h"
#include "stb_ds.h"
#include <unistd.h>

#define ERROR_BUF_SIZE 65536
static char g_error_buf[ERROR_BUF_SIZE] = {};

enum error_t { error_max_iters = 1 };

struct EvalState_s {
  uint root;
  uint *nodes;
  uint nodes_size;
  uint *stack;

  i8 error_code;
  char *error;
};

// TODO: error checking
uint eval_init(struct EvalState_s **state, uint root, const uint *nodes,
          uint nodes_size) {
  *state = malloc(sizeof(struct EvalState_s));
  memset(*state, 0, sizeof(struct EvalState_s));
  (*state)->root = root;
  (*state)->nodes_size = nodes_size;
  for (uint i = 0; i < nodes_size; ++i) {
    stbds_arrput((*state)->nodes, nodes[i]);
  }
  (*state)->error = g_error_buf;
  return 0;
}

uint eval_free(EvalState *state) {
  stbds_arrfree((*state)->nodes);
  stbds_arrfree((*state)->stack);
  free(*state);
  *state = NULL;
  return 0;
}

struct NodeWithPos {
  uint node;
  sint shift;
  sint pos;
};

static struct NodeWithPos fetch_node(const uint *nodes, uint i) {
  const uint n = nodes[i];
  return (struct NodeWithPos){.node = n, .shift = i, .pos = i};
}

static struct NodeWithPos fetch_lhs(uint *nodes, struct NodeWithPos node) {
  const uint shift = node_lhs(node.node);
  if (shift == node_new_invalid()) {
    return (struct NodeWithPos){node_new_invalid(), node_new_invalid(),
                                node_new_invalid()};
  }
  const uint i = node.pos + shift;
  return (struct NodeWithPos){.node = nodes[i], .shift = shift, .pos = i};
}

static struct NodeWithPos fetch_rhs(uint *nodes, struct NodeWithPos node) {
  const uint shift = node_rhs(node.node);
  if (shift == node_new_invalid()) {
    return (struct NodeWithPos){node_new_invalid(), node_new_invalid(),
                                node_new_invalid()};
  }
  const uint i = node.pos + shift;
  return (struct NodeWithPos){.node = nodes[i], .shift = shift, .pos = i};
}

uint eval_step(struct EvalState_s *state) {
  const uint root_pos = stbds_arrpop(state->stack);
  const struct NodeWithPos root = fetch_node(state->nodes, root_pos);
  if (node_tag(root.node) == NODE_APP) {
    const struct NodeWithPos delta = fetch_lhs(state->nodes, root);
    const struct NodeWithPos arg = fetch_rhs(state->nodes, root);
    // NOTE: Comma in following rules used as infix application operator
    // 1. ^, z -> ^ z
    if (node_tag(delta.node) == NODE_TREE &&                //
        node_lhs(delta.node) == (sint)node_new_invalid() && //
        node_rhs(delta.node) == (sint)node_new_invalid()) {
      state->nodes[delta.pos] =
          node_new_tree(arg.pos - delta.pos, node_new_invalid());
      return 0;
    }
    // 2. ^ y, z -> ^ y z
    if (node_tag(delta.node) == NODE_TREE && //
        node_rhs(delta.node) == (sint)node_new_invalid()) {
      state->nodes[delta.pos] =
          node_new_tree(node_lhs(delta.node), arg.pos - delta.pos);
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
      state->root = delta.pos;
      stbds_arrput(state->stack, state->root);
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
      stbds_arrput(state->nodes,
                   node_new_app(app_pos - y.pos, app_pos - z.pos));
      stbds_arrput(state->stack, app_pos);

      const sint app_lhs_pos = app_pos;
      app_pos = stbds_arrlen(state->nodes);
      stbds_arrput(state->nodes,
                   node_new_app(app_pos - x.pos, app_pos - z.pos));
      stbds_arrput(state->stack, app_pos);

      const sint app_rhs_pos = app_pos;
      app_pos = stbds_arrlen(state->nodes);
      stbds_arrput(state->nodes,
                   node_new_app(app_pos - app_lhs_pos, app_pos - app_rhs_pos));
      stbds_arrput(state->stack, app_pos);
      state->root = app_pos;
      return 0;
    }

    // NOTE: The following is the triage-calculus, baking the rules of triage
    // into reduction itself 5 (3a). ^ (^ a b) c ^ -> a
    /* if (node_tag(arg.node) == NODE_APP) { */
    /*     stbds_arrput(state->stack, arg.pos); */
    /*     stbds_arrput(state->stack, root_pos); */
    /*     return 0; */
    /* } */
    /* assert(node_tag(arg.node) == NODE_TREE); */
    /* if (node_tag(delta.node) == NODE_TREE &&      // */
    /*     node_tag(delta_left.node) == NODE_TREE && // */
    /* ) { */
    /*   const struct NodeWithPos w = fetch_lhs(state->nodes, delta_left); */
    /*   const struct NodeWithPos x = fetch_rhs(state->nodes, delta_left); */
    /*   const struct NodeWithPos z = arg; */
    /* } */
  }
  assert(node_tag(root.node) == NODE_TREE);
  return 0;
}

#define MAX_ITERS (uint)65535

uint eval_eval(struct EvalState_s *state) {
  uint i = 0;
  return 0;

  while (1) {
    if (i >= MAX_ITERS) {
      state->error_code = error_max_iters;
      sprintf(g_error_buf, "%sMaximum iteration count %zu exceeded\n",
              g_error_buf, MAX_ITERS);
      return 0;
    }

    if (node_tag(state->nodes[state->root]) == NODE_APP) {
      stbds_arrput(state->stack, state->root);
    }
    if (stbds_arrlen(state->stack) == 0) {
      return 0;
    }
    // state->root = stbds_arrpop(state->stack);
    if (eval_step(state)) {
      return 0;
    }

    i++;
  }
  return 0;
}

uint eval_get_error(EvalState state, uint* code, char** error) {
    *code = state->error_code;
    *error = state->error;
    return 0;
}
