

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "common.h"
#include "stb_ds.h"
#include <unistd.h>

#define ERROR_BUF_SIZE 65536
static char g_error_buf[ERROR_BUF_SIZE] = {};

#define ERROR_PARSE 1
#define ERROR_STACK_UNDERFLOW 2
#define ERROR_GENERIC 127

#define EVAL_TAG_NUMBER 0
#define EVAL_TAG_INDEX 1

struct EvalState_impl {
  Allocator cells;
  size_t *stack;

  int8_t error_code;
  const char *error;
};

uint eval_init(EvalState *state, const char *program) {
  EvalState s = calloc(1, sizeof(struct EvalState_impl));
  if (s == NULL) {
    return ERROR_VALUE;
  }
  s->error = g_error_buf;

  uint res = eval_cells_init(&s->cells, 4);
  if (res == ERROR_VALUE) {
    return res;
  }
  res = eval_encode_parse(s->cells, program);
  if (res == ERROR_VALUE) {
    s->error_code = ERROR_PARSE;
    sprintf(g_error_buf, "failed to parse %s", program);
    return res;
  }
  stbds_arrput(s->stack, 0);
  *state = s;
  return 0;
}

uint eval_free(EvalState *state) {
  EvalState s = *state;
  eval_cells_free(&s->cells);
  free(s);
  *state = NULL;
  return 0;
}

typedef enum {
  deref_error,
  deref_unchanged,
  deref_changed,
} deref_t;

static deref_t deref(Allocator cells, size_t *root, uint8_t root_cell) {
  if (root_cell != EVAL_NATIVE) {
    return deref_unchanged;
  }

  word_t word = eval_cells_get_word(cells, *root);
  uint8_t tag = GET_TAG(word);
  if (tag != EVAL_TAG_INDEX) {
    return deref_error;
  }
  *root = GET_PAYLOAD(word);
  return deref_changed;
}

static uint first_rule(EvalState state, uint root, bool *matched) {
  uint root_cell = eval_cells_get(state->cells, root);
  if (root_cell != EVAL_TREE) {
    return 0;
  }
  root++;
  // TODO: we need while here to dereference N references
  // and we also need cycle detection :|
  // deref_t deref_result = deref(state->cells, &root, root_cell);
  // if (deref_result == deref_error) {
  //   state->error_code = ERROR_GENERIC;
  //   g_error_buf[0] = '\0';
  //   return ERROR_VALUE;
  // }
  

  *matched = true;
  return 1;
}

uint eval_step(EvalState state, bool *matched) {
  if (stbds_arrlenu(state->stack) == 0) {
    state->error_code = ERROR_STACK_UNDERFLOW;
    g_error_buf[0] = '\0';
    return ERROR_VALUE;
  }
  size_t root = stbds_arrpop(state->stack);
  uint root_cell = eval_cells_get(state->cells, root);
  if (root_cell == ERROR_VALUE) {
    state->error_code = ERROR_GENERIC;
    g_error_buf[0] = '\0';
    return ERROR_VALUE;
  }
  if (root_cell != EVAL_APPLY) {
    return 0;
  }

  uint next_node = root + 1;
  deref_t deref_result = deref(state->cells, &next_node, root_cell);
  if (deref_result == deref_error) {
    state->error_code = ERROR_GENERIC;
    sprintf(g_error_buf, "cannot apply to a value [%zu] %zu", root,
            eval_cells_get_word(state->cells, next_node));
    return ERROR_VALUE;
  }
  if (deref_result == deref_changed) {
    stbds_arrput(state->stack, next_node);
    next_node = eval_step(state, matched);
    stbds_arrput(state->stack, next_node);
    return next_node;
  }

  uint subtree_len = 0;
  subtree_len = first_rule(state, next_node, matched);
  if (*matched) {
    return subtree_len;
  }

  return 1;
}

#if 0

struct EvalState_impl {
  uint root;
  uint *nodes;
  uint nodes_size;
  uint *stack;

  i8 error_code;
  char *error;
};

// TODO: error checking
uint eval_init(struct EvalState_impl **state, uint root, const uint *nodes,
               uint nodes_size) {
  *state = malloc(sizeof(struct EvalState_impl));
  memset(*state, 0, sizeof(struct EvalState_impl));
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

uint eval_step(struct EvalState_impl *state) {
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

uint eval_eval(struct EvalState_impl *state) {
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

uint eval_get_error(EvalState state, uint *code, char **error) {
  *code = state->error_code;
  *error = state->error;
  return 0;
}

#endif