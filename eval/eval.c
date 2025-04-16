

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
#define ERROR_DEREF_NONREF 3
#define ERROR_INVALID_CELL 4
#define ERROR_INVALID_WORD 5
#define ERROR_REF_TO_REF 6
#define ERROR_APPLY_TO_VALUE 7
#define ERROR_REF_EXPECTED 8
#define ERROR_GENERIC 127

struct EvalState_impl {
  Allocator cells;
  size_t *stack;
  word_t *free_bitmap;
  size_t free_capacity;

  uint8_t error_code;
  const char *error;
};

uint eval_init(EvalState *state, const char *program) {
  EvalState s = calloc(1, sizeof(struct EvalState_impl));
  if (s == NULL) {
    return ERROR_VALUE;
  }
  s->error = g_error_buf;

  uint cells_capacity = 4;
  uint res = eval_cells_init(&s->cells, cells_capacity);
  if (res == ERROR_VALUE) {
    return res;
  }
  res = eval_encode_parse(s->cells, program);
  if (res == ERROR_VALUE) {
    s->error_code = ERROR_PARSE;
    snprintf(g_error_buf, ERROR_BUF_SIZE, "failed to parse %s", program);
    return res;
  }
  stbds_arrput(s->stack, 0);
  s->free_capacity = BITMAP_SIZE(cells_capacity * CELLS_PER_WORD);
  s->free_bitmap = calloc(1, s->free_capacity * sizeof(word_t));
  size_t i = 0;
  while (eval_cells_is_set(s->cells, i)) {
    _bitmap_set_bit(s->free_bitmap, i, 1);
    i++;
  }
  *state = s;
  return 0;
}

uint eval_free(EvalState *state) {
  EvalState s = *state;
  eval_cells_free(&s->cells);
  stbds_arrfree(s->stack);
  free(s->free_bitmap);
  free(s);
  *state = NULL;
  return 0;
}

typedef struct {
  size_t sibling;
  size_t result;
} EvalResult;

static inline EvalResult new_eval_result(size_t sibling, size_t result) {
  return (EvalResult){
      .sibling = sibling,
      .result = result,
  };
}

#define EXPECT(cond, code, msg)                                                \
  if (!(cond)) {                                                               \
    state->error_code = (code);                                                \
    snprintf(g_error_buf, ERROR_BUF_SIZE, "%s %s '%s'", #cond, #code, msg);    \
    goto cleanup;                                                              \
  }

#define CHECK(state)                                                           \
  if (state->error_code) {                                                     \
    goto cleanup;                                                              \
  }

#define GET_CELL(index)                                                        \
  uint index##_cell = eval_cells_get(state->cells, index);                     \
  EXPECT(index##_cell != ERROR_VALUE, ERROR_INVALID_CELL, "");

#define SET_CELL(index, value)                                                 \
  do {                                                                         \
    uint res = eval_cells_set(state->cells, index, value);                     \
    EXPECT(res != ERROR_VALUE, ERROR_GENERIC, "");                             \
  } while (0)

#define GET_WORD(index)                                                        \
  uint index##_word = eval_cells_get_word(state->cells, index);                \
  EXPECT(index##_cell != ERROR_VALUE, ERROR_INVALID_WORD, "");

#define SET_WORD(index, tag, payload)                                          \
  do {                                                                         \
    ssize_t word = 0;                                                          \
    word = EVAL_SET_TAG(word, tag);                                            \
    word = EVAL_SET_PAYLOAD(word, payload);                                    \
    uint res = eval_cells_set_word(state->cells, index, word);                 \
    EXPECT(res != ERROR_VALUE, ERROR_GENERIC, "");                             \
  } while (0)

#define DEREF(index)                                                           \
  if (index##_cell == EVAL_NATIVE) {                                           \
    GET_WORD(index);                                                           \
    uint8_t tag = EVAL_GET_TAG(index##_word);                                  \
    EXPECT(tag == EVAL_TAG_INDEX, ERROR_DEREF_NONREF, "");                     \
    index += (ssize_t)EVAL_GET_PAYLOAD(index##_word);                          \
    size_t new_index = index;                                                  \
    GET_CELL(new_index)                                                        \
    if (new_index_cell == EVAL_NATIVE) {                                       \
      GET_WORD(new_index)                                                      \
      uint8_t tag = EVAL_GET_TAG(new_index##_word);                            \
      EXPECT(tag != EVAL_TAG_INDEX, ERROR_REF_TO_REF, "");                     \
    }                                                                          \
  }

static inline bool is_ref(EvalState state, uint index) {
  GET_CELL(index)
  if (index_cell == EVAL_NATIVE) {
    GET_WORD(index)
    uint8_t tag = EVAL_GET_TAG(index_word);
    return tag == EVAL_TAG_INDEX;
  }
cleanup:
  return false;
}

static inline bool is_opaque(EvalState state, uint index) {
  GET_CELL(index)
  if (index_cell == EVAL_TREE) {
    return true;
  }
  if (index_cell == EVAL_NATIVE) {
    // GET_WORD(index)
    // uint8_t tag = EVAL_GET_TAG(index_word);
    // return tag != EVAL_TAG_FUNC;
    return true;
  }
cleanup:
  return false;
}

static size_t next_n_vacant_cells(EvalState state, size_t n) {
  assert(n != 0);
  for (size_t w = 0; w < state->free_capacity; ++w) {
    if (state->free_bitmap[w] != (word_t)-1) {
      word_t mask = (1U << n) - 1;
      for (size_t b = 0; b < BITS_PER_WORD - n + 1; ++b) {
        if ((state->free_bitmap[w] & mask) == 0) {
          return (w * BITS_PER_WORD) + b;
        }
        mask <<= 1;
      }
    }
  }
  size_t old_cap = state->free_capacity;
  state->free_capacity *= 2;
  state->free_bitmap = realloc(state->free_bitmap, state->free_capacity);
  memset(state->free_bitmap + old_cap, 0, old_cap);
  return next_n_vacant_cells(state, n);
}

// NOTE: references are second class, we can't have references to references!
// Namings: A   B   C   D   E   F   G   H   I       P   Q   R   S   T   U   V
// Rule 1 : $   ^   ^   *   *   X   Y           ->  X
// Rule 2 : $   ^   ^   X   *   Y   Z           ->  $   $   X   Z   $   Y   Z
// Rule 3a: $   ^   ^   W   X   Y   ^   *   *   ->  W
// Rule 3b: $   ^   ^   W   X   Y   ^   U   *   ->  $   X   U
// Rule 3c: $   ^   ^   W   X   Y   ^   U   V   ->  $   $   Y   U   V
// Rule 4 : $   N   X, where N is native function and X is a arbitrary value
// (tree or native)
EvalResult eval_step_impl(EvalState state) {
  EvalResult result = {};
  EXPECT(stbds_arrlenu(state->stack) > 0, ERROR_STACK_UNDERFLOW, "");

  size_t root = stbds_arrpop(state->stack);
  size_t A = root;
  GET_CELL(A)
  DEREF(A)

  if (A_cell != EVAL_APPLY) {
    return new_eval_result(ERROR_VALUE, root);
  }

  size_t B = A + 1;
  GET_CELL(B)
  DEREF(B)

  EXPECT(B != EVAL_NIL, ERROR_GENERIC, "")
  if (B == EVAL_NATIVE) {
    GET_WORD(B)
    uint8_t tag = EVAL_GET_TAG(B_word);
    EXPECT(tag == EVAL_TAG_FUNC, ERROR_APPLY_TO_VALUE, "")
    assert(false); // TODO: this
  } else if (B_cell == EVAL_APPLY) {
    assert(false);
    stbds_arrpush(state->stack, B);
    EvalResult lhs_result = eval_step_impl(state);
    CHECK(state)
    size_t rhs = lhs_result.sibling;
    GET_CELL(rhs)
    stbds_arrpush(state->stack, rhs);
    EvalResult rhs_result = eval_step_impl(state);
    assert(rhs_result.sibling == ERROR_VALUE);
    // TODO create a $ lhs rhs and put it on the stack
  }
  assert(B == EVAL_TREE);

  bool matched = false;
  size_t new_root = root;
  do {
    size_t C = B + 1;
    GET_CELL(C)
    DEREF(C)
    size_t D = C + 1;
    GET_CELL(D)
    size_t E = C + 2;
    GET_CELL(E)
    if (!(D_cell == EVAL_NIL && E_cell == EVAL_NIL)) {
      break;
    }
    matched = true;
    size_t F = E + 1;
    new_root = F;
  } while (0);
  CHECK(state)
  if (matched) {
    result.result = new_root;
    goto cleanup;
  }
  matched = false;

  do {
    size_t C = B + 1;
    GET_CELL(C)
    DEREF(C)
    size_t D = C + 1;
    GET_CELL(D)
    EXPECT(is_ref(state, D), ERROR_REF_EXPECTED,
           "currently cannot get to other sibling if not ref :(");
    size_t E = D + 2;
    GET_CELL(E)
    if (!(is_opaque(state, D) && E_cell == EVAL_NIL)) {
      break;
    }

    size_t F = E + 1;
    EXPECT(is_ref(state, F), ERROR_REF_EXPECTED,
           "currently cannot get to other sibling if not ref :(");
    size_t G = F + 2;
    size_t Q = next_n_vacant_cells(state, 3);
    size_t R = Q + 1;
    size_t S = R + 1;
    // TODO: undefined behaviour when trying to shift the negative value :P
    word_t D_ref = (ssize_t)D - (ssize_t)Q;
    word_t G_ref = (ssize_t)G - (ssize_t)Q;

    SET_CELL(Q, EVAL_APPLY);
    SET_CELL(R, EVAL_NATIVE);
    SET_CELL(S, EVAL_NATIVE);
    SET_WORD(R, EVAL_TAG_INDEX, D_ref);
    SET_WORD(S, EVAL_TAG_INDEX, G_ref);
  } while (0);

cleanup:
  return result;
}

uint eval_step(EvalState state) { return eval_step_impl(state).result; }

Allocator eval_get_memory(EvalState state) { return state->cells; }

uint8_t eval_get_error(EvalState state, const char **message) {
  *message = state->error;
  return state->error_code;
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
      snprintf(g_error_buf, ERROR_BUF_SIZE, "%sMaximum iteration count %zu exceeded\n",
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
