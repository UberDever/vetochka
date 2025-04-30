

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "vendor/stb_ds.h"

#define ERROR_BUF_SIZE 65536
static char g_error_buf[ERROR_BUF_SIZE] = {};

void _errbuf_clear() {
  g_error_buf[0] = '\0';
}

void _errbuf_write(const char* format, ...) {
  va_list args;
  va_start(args, format);

  size_t current_length = strlen(g_error_buf);
  size_t remaining_space = ERROR_BUF_SIZE - current_length;

  if (remaining_space > 0) {
    vsnprintf(g_error_buf + current_length, remaining_space, format, args);
  }

  va_end(args);
}

#define ERROR_PARSE           1
#define ERROR_STACK_UNDERFLOW 2
#define ERROR_DEREF_NONREF    3
#define ERROR_INVALID_CELL    4
#define ERROR_INVALID_WORD    5
#define ERROR_REF_TO_REF      6
#define ERROR_APPLY_TO_VALUE  7
#define ERROR_REF_EXPECTED    8
#define ERROR_INVALID_TREE    9
#define ERROR_GENERIC         127

sint eval_init(EvalState* state) {
  EvalState s = calloc(1, sizeof(struct EvalState_impl));
  if (s == NULL) {
    return ERR_VAL;
  }
  s->error = g_error_buf;

  size_t cells_capacity = 4;
  sint res = eval_cells_init(&s->cells, cells_capacity);
  if (res == ERR_VAL) {
    return res;
  }

  s->free_capacity = BITMAP_SIZE(cells_capacity * CELLS_PER_WORD);
  s->free_bitmap = calloc(1, s->free_capacity * sizeof(u64));
  *state = s;
  return 0;
}

sint eval_free(EvalState* state) {
  EvalState s = *state;
  eval_cells_free(&s->cells);
  stbds_arrfree(s->apply_stack);
  stbds_arrfree(s->result_stack);
  stbds_arrfree(s->match_stack);
  free(s->free_bitmap);
  free(s);
  *state = NULL;
  return 0;
}

sint _eval_reset_cells(EvalState state) {
  sint err = eval_cells_reset(state->cells);
  memset(state->free_bitmap, 0, state->free_capacity * sizeof(*state->free_bitmap));
  return err;
}

sint eval_reset(EvalState state) {
  if (!state) {
    return ERR_VAL;
  }
  sint err = _eval_reset_cells(state);
  if (err) {
    return err;
  }
  stbds_arrsetlen(state->apply_stack, 0);
  state->error_code = 0;
  return 0;
}

#define EXPECT(cond, code, msg)                                                                    \
  if (!(cond)) {                                                                                   \
    goto cleanup;                                                                                  \
  }

#define ASSERT(cond, code, msg)                                                                    \
  if (!(cond)) {                                                                                   \
    state->error_code = (code);                                                                    \
    _errbuf_write("%s %s %s\n", #cond, #code, msg);                                                \
    goto cleanup;                                                                                  \
  }

#define CHECK(state)                                                                               \
  if (state->error_code) {                                                                         \
    goto cleanup;                                                                                  \
  }

static size_t next_n_vacant_cells(EvalState state, size_t n) {
  assert(n != 0);
  for (size_t w = 0; w < state->free_capacity; ++w) {
    if (state->free_bitmap[w] != (u64)-1) {
      u64 mask = (1ULL << n) - 1;
      for (size_t b = 0; b < BITS_PER_WORD - n + 1; ++b) {
        if ((state->free_bitmap[w] & mask) == 0) {
          state->free_bitmap[w] |= mask;
          return (w * BITS_PER_WORD) + b;
        }
        mask <<= 1;
      }
    }
  }
  size_t old_cap = state->free_capacity;
  state->free_capacity *= 2;
  state->free_bitmap =
      realloc(state->free_bitmap, state->free_capacity * sizeof(*state->free_bitmap));
  memset(state->free_bitmap + old_cap, 0, old_cap * sizeof(*state->free_bitmap));
  return next_n_vacant_cells(state, n);
}

// static inline bool is_terminal(u8 cell) {
//   return cell == SIGIL_NIL || cell == SIGIL_REF;
// }

// TODO: need to verify the tree for validity before evaluation

// NOTE: Two following algorithms: given a root, get the corresponding node index
// works only for tree nodes, because others are essentially terminals
// automatically dereferences references
// assumes a valid tree
// returns the passed index if no corresponding node exists

static inline size_t get_left_node(EvalState state, size_t root_index) {
  sint root_cell = eval_cells_get(state->cells, root_index);
  if (root_cell == ERR_VAL || root_cell != SIGIL_TREE) {
    return root_index;
  }

  size_t lhs_index = root_index + 1;
  sint cell = eval_cells_get(state->cells, lhs_index);
  assert(cell != ERR_VAL);
  if (cell == SIGIL_NIL) {
    return lhs_index;
  }
  // reference node
  if (cell == SIGIL_REF) {
    sint l = eval_cells_get(state->cells, lhs_index + 1);
    sint r = eval_cells_get(state->cells, lhs_index + 2);
    if (l == SIGIL_NIL && r == SIGIL_NIL) {
      sint ref = eval_cells_get_word(state->cells, lhs_index);
      return ref + lhs_index;
    }
  }
  return lhs_index;
}

static inline size_t get_right_node(EvalState state, size_t root_index) {
  stbds_arrsetlen(state->match_stack, 0);
  sint root_cell = eval_cells_get(state->cells, root_index);
  if (root_cell == ERR_VAL || root_cell != SIGIL_TREE) {
    return root_index;
  }

  size_t cur = root_index + 1;
  while (true) {
    if (stbds_arrlenu(state->match_stack) == 1 && state->match_stack[0] == SIGIL_NIL) {
      break;
    }

    if (stbds_arrlenu(state->match_stack) >= 3) {
      size_t last = stbds_arrlenu(state->match_stack) - 1;
      u8 rhs = state->match_stack[last];
      u8 lhs = state->match_stack[last - 1];
      if ((lhs == SIGIL_NIL && rhs == SIGIL_NIL) || (lhs == SIGIL_REF && rhs == SIGIL_NIL)
          || (lhs == SIGIL_REF && rhs == SIGIL_REF)) {
        stbds_arrpop(state->match_stack); // rhs
        stbds_arrpop(state->match_stack); // lhs
        stbds_arrpop(state->match_stack); // root
        stbds_arrput(state->match_stack, SIGIL_NIL);
        continue;
      }
    }

    sint cell = eval_cells_get(state->cells, cur);
    assert(cell != ERR_VAL);
    stbds_arrput(state->match_stack, cell);
    cur++;
  }

  // reference node
  sint rhs_cell = eval_cells_get(state->cells, cur);
  if (rhs_cell == SIGIL_REF) {
    sint l = eval_cells_get(state->cells, cur + 1);
    sint r = eval_cells_get(state->cells, cur + 2);
    if (l == SIGIL_NIL && r == SIGIL_NIL) {
      sint ref = eval_cells_get_word(state->cells, cur);
      return ref + cur;
    }
  }

  return cur;
}

static inline size_t dereference(EvalState state, size_t index) {
  sint index_cell = eval_cells_get(state->cells, index);
  if (index_cell == SIGIL_REF) {
    sint index_word = eval_cells_get_word(state->cells, index);
    ASSERT(index_word != ERR_VAL, ERROR_GENERIC, "");
    index += index_word;
  }
cleanup:
  return index;
}

static inline bool is_terminal(EvalState state, size_t index) {
  sint root = eval_cells_get(state->cells, index);
  sint left = eval_cells_get(state->cells, index + 1);
  sint right = eval_cells_get(state->cells, index + 2);

  if (root == ERR_VAL || left == ERR_VAL || right == ERR_VAL) {
    return false;
  }

  bool result = false;
  // NOTE: leaf
  result |= root == SIGIL_TREE && left == SIGIL_NIL && right == SIGIL_NIL;
  // NOTE: stem
  result |= root == SIGIL_TREE && left == SIGIL_TREE && right == SIGIL_NIL;
  // NOTE: fork
  result |= root == SIGIL_TREE && left == SIGIL_TREE && right == SIGIL_TREE;
  // NOTE: ref
  result |= root == SIGIL_REF && left == SIGIL_NIL && right == SIGIL_NIL;
  // NOTE: native
  result |= root == SIGIL_REF && left == SIGIL_REF && right == SIGIL_NIL;
  return result;
}

// clang-format off
// Apply forms:
// index:     0   1   2   3   4   5   6   7
// no rule    F   z
// no rule    ^   A   y   z
// 1,2        ^   w   x   y   z
// 1,2,3      ^   ^   w   x   y   ^   u   v
// 0.a        ^   *   *   z
// 0.b        ^   ^   *   *   *   z

// clang-format on

sint eval_step(EvalState state) {
  if (stbds_arrlenu(state->apply_stack) == 0) {
    return true;
  }

  bool was_apply = false;
  while (stbds_arrlenu(state->apply_stack) > 0) {
    size_t i = stbds_arrpop(state->apply_stack);
    if (i == SIZE_MAX) {
      was_apply = true;
      break;
    }
    stbds_arrput(state->result_stack, dereference(state, i));
  }

  if (!was_apply) {
    return true;
  }

  ASSERT(stbds_arrlenu(state->result_stack) >= 2, ERROR_STACK_UNDERFLOW, "");

  size_t F = stbds_arrpop(state->result_stack);
  size_t z = stbds_arrpop(state->result_stack);
  sint F_cell = eval_cells_get(state->cells, F);
  ASSERT(F_cell == SIGIL_TREE, ERROR_GENERIC, "");

  size_t A = get_left_node(state, F);
  ASSERT(A != F, ERROR_INVALID_TREE, "");
  sint A_cell = eval_cells_get(state->cells, A);

  size_t y = get_right_node(state, F);
  ASSERT(y != F, ERROR_INVALID_TREE, "");
  size_t w = get_left_node(state, A);
  size_t x = get_right_node(state, A);
  sint y_cell = eval_cells_get(state->cells, y);
  sint w_cell = eval_cells_get(state->cells, w);
  sint x_cell = eval_cells_get(state->cells, x);

  // rule 0.a
  if (A_cell == SIGIL_NIL && y_cell == SIGIL_NIL) {
    size_t new = next_n_vacant_cells(state, 5);
    size_t ref = new + 1;
    eval_cells_set(state->cells, new + 0, SIGIL_TREE);
    eval_cells_set(state->cells, ref, SIGIL_REF);
    eval_cells_set(state->cells, new + 2, SIGIL_NIL);
    eval_cells_set(state->cells, new + 3, SIGIL_NIL);
    eval_cells_set(state->cells, new + 4, SIGIL_NIL);
    eval_cells_set_word(state->cells, ref, z - ref);
    stbds_arrput(state->apply_stack, new);
    return false;
  }
  ASSERT(is_terminal(state, A), ERROR_GENERIC, "");
  ASSERT(w != A, ERROR_INVALID_TREE, "");
  ASSERT(x != A, ERROR_INVALID_TREE, "");

  // rule 0.b
  if (w_cell == SIGIL_NIL && x_cell == SIGIL_NIL && y_cell == SIGIL_NIL) {
    size_t new = next_n_vacant_cells(state, 7);
    size_t ref1 = new + 1;
    size_t ref2 = new + 4;
    eval_cells_set(state->cells, new + 0, SIGIL_TREE);
    eval_cells_set(state->cells, ref1, SIGIL_REF);
    eval_cells_set(state->cells, new + 2, SIGIL_NIL);
    eval_cells_set(state->cells, new + 3, SIGIL_NIL);
    eval_cells_set(state->cells, ref2, SIGIL_REF);
    eval_cells_set(state->cells, new + 5, SIGIL_NIL);
    eval_cells_set(state->cells, new + 6, SIGIL_NIL);
    eval_cells_set_word(state->cells, ref2, A - ref1);
    eval_cells_set_word(state->cells, ref2, z - ref2);
    stbds_arrput(state->apply_stack, new);
    return false;
  }

  if (w_cell == SIGIL_NIL && x_cell == SIGIL_NIL) {
    // rule 1
    stbds_arrpush(state->apply_stack, y);
    return false;
  }
  if (w_cell == SIGIL_NIL && x_cell != SIGIL_NIL) {
    // NOTE: this tree is unrepresentable in tree-calculus
    ASSERT(false, ERROR_INVALID_TREE, "");
  }
  if (w_cell != SIGIL_NIL && x_cell == SIGIL_NIL) {
    // rule 2
    x = w; // NOTE: because I've unified all rules together, names have clashed
    stbds_arrpush(state->apply_stack, TOKEN_APPLY);
    stbds_arrpush(state->apply_stack, TOKEN_APPLY);
    stbds_arrpush(state->apply_stack, x);
    stbds_arrpush(state->apply_stack, z);
    stbds_arrpush(state->apply_stack, TOKEN_APPLY);
    stbds_arrpush(state->apply_stack, y);
    stbds_arrpush(state->apply_stack, z);
    return false;
  }
  if (w_cell != SIGIL_NIL && x_cell != SIGIL_NIL) {
    // rule 3(?)
    size_t u = get_left_node(state, z);
    ASSERT(u != z, ERROR_INVALID_TREE, "");
    size_t v = get_right_node(state, z);
    ASSERT(v != z, ERROR_INVALID_TREE, "");
    sint u_cell = eval_cells_get(state->cells, u);
    sint v_cell = eval_cells_get(state->cells, v);
    ASSERT(u_cell != ERR_VAL, ERROR_INVALID_TREE, "");
    ASSERT(v_cell != ERR_VAL, ERROR_INVALID_TREE, "");
    if (u_cell == SIGIL_NIL && v_cell == SIGIL_NIL) {
      // rule 3a
      stbds_arrpush(state->apply_stack, w);
      return false;
    }
    if (u_cell == SIGIL_NIL && v_cell != SIGIL_NIL) {
      // NOTE: this tree is unrepresentable in tree-calculus
      ASSERT(false, ERROR_INVALID_TREE, "");
    }
    if (u_cell != SIGIL_NIL && v_cell == SIGIL_NIL) {
      // rule 3b
      stbds_arrpush(state->apply_stack, TOKEN_APPLY);
      stbds_arrpush(state->apply_stack, x);
      stbds_arrpush(state->apply_stack, u);
      return false;
    }
    if (u_cell != SIGIL_NIL && v_cell != SIGIL_NIL) {
      // rule 3c
      stbds_arrpush(state->apply_stack, TOKEN_APPLY);
      stbds_arrpush(state->apply_stack, TOKEN_APPLY);
      stbds_arrpush(state->apply_stack, y);
      stbds_arrpush(state->apply_stack, u);
      stbds_arrpush(state->apply_stack, v);
      return false;
    }
  }

cleanup:
  return false;
}

#undef CALCULATE_OFFSET
#undef EXPECT
#undef ASSERT
#undef CHECK
