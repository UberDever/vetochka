

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
#define ERROR_GENERIC         127

struct EvalState_impl {
  Allocator cells;
  size_t* stack;
  u64* free_bitmap;
  size_t free_capacity;

  uint8_t error_code;
  const char* error;
};

sint eval_init(EvalState* state, const char* program) {
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
  res = eval_encode_parse(s->cells, program);
  if (res == ERR_VAL) {
    s->error_code = ERROR_PARSE;
    _errbuf_write("failed to parse %s\n", program);
    return res;
  }
  stbds_arrput(s->stack, 0);
  s->free_capacity = BITMAP_SIZE(cells_capacity * CELLS_PER_WORD);
  s->free_bitmap = calloc(1, s->free_capacity * sizeof(u64));
  size_t i = 0;
  while (eval_cells_is_set(s->cells, i)) {
    _bitmap_set_bit(s->free_bitmap, i, 1);
    i++;
  }
  *state = s;
  return 0;
}

sint eval_free(EvalState* state) {
  EvalState s = *state;
  eval_cells_free(&s->cells);
  stbds_arrfree(s->stack);
  free(s->free_bitmap);
  free(s);
  *state = NULL;
  return 0;
}

typedef struct {
  sint sibling;
  size_t result;
} EvalResult;

static inline EvalResult new_eval_result(sint sibling, size_t result) {
  return (EvalResult){
      .sibling = sibling,
      .result = result,
  };
}

#define EXPECT(cond, code, msg)                                                                    \
  if (!(cond)) {                                                                                   \
    state->error_code = (code);                                                                    \
    _errbuf_write("%s %s %s\n", #cond, #code, msg);                                                \
    goto cleanup;                                                                                  \
  }

#define CHECK(state)                                                                               \
  if (state->error_code) {                                                                         \
    goto cleanup;                                                                                  \
  }

#define STATE_GET_CELL(index) GET_CELL(state->cells, index)

#define STATE_SET_CELL(index, value)                                                               \
  do {                                                                                             \
    sint res = eval_cells_set(state->cells, index, value);                                         \
    _bitmap_set_bit(state->free_bitmap, index, 1);                                                 \
    EXPECT(res != ERR_VAL, ERROR_GENERIC, "");                                                     \
  } while (0)

#define STATE_GET_WORD(index) GET_WORD(state->cells, index)

#define STATE_SET_WORD(index, tag, payload)                                                        \
  do {                                                                                             \
    sint word = 0;                                                                                 \
    word = eval_tv_set_tag(word, tag);                                                             \
    word = eval_tv_set_payload_signed(word, payload);                                              \
    sint res = eval_cells_set_word(state->cells, index, word);                                     \
    EXPECT(res != ERR_VAL, ERROR_GENERIC, "");                                                     \
  } while (0)

#define STATE_DEREF(index) DEREF(state->cells, index)

// NOTE: references are second class, we can't have references to references!
// NOTE: references are represented as single # node with corresponding tagged word node
static inline bool _eval_is_ref(EvalState state, size_t index) {
  STATE_GET_CELL(index)
  return (index_cell == EVAL_REF);
cleanup:
  return false;
}

static inline bool is_opaque(EvalState state, size_t index) {
  STATE_GET_CELL(index)
  if (index_cell == EVAL_TREE) {
    return true;
  }
  // if (index_cell == EVAL_NATIVE) {
  // GET_WORD(index)
  // uint8_t tag = EVAL_GET_TAG(index_word);
  // return tag != EVAL_TAG_FUNC;
  // return true;
  // }
cleanup:
  return false;
}

static size_t next_n_vacant_cells(EvalState state, size_t n) {
  assert(n != 0);
  for (size_t w = 0; w < state->free_capacity; ++w) {
    if (state->free_bitmap[w] != (u64)-1) {
      u64 mask = (1U << n) - 1;
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
  STATE_GET_CELL(A)
  STATE_DEREF(A)

  if (A_cell != EVAL_APPLY) {
    return new_eval_result(ERR_VAL, root);
  }

  size_t B = A + 1;
  STATE_GET_CELL(B)
  STATE_DEREF(B)

  EXPECT(B != EVAL_NIL, ERROR_GENERIC, "")
  /*if (B == EVAL_NATIVE) {
    STATE_GET_WORD(B)
    u8 tag = eval_tv_get_tag(B_word);
    EXPECT(tag == EVAL_TAG_FUNC, ERROR_APPLY_TO_VALUE, "")
    assert(false); // TODO: this
  } else */
  if (B_cell == EVAL_APPLY) {
    assert(false);
    stbds_arrpush(state->stack, B);
    EvalResult lhs_result = eval_step_impl(state);
    CHECK(state)
    size_t rhs = lhs_result.sibling;
    STATE_GET_CELL(rhs)
    stbds_arrpush(state->stack, rhs);
    EvalResult rhs_result = eval_step_impl(state);
    assert(rhs_result.sibling == ERR_VAL);
    // TODO create a $ lhs rhs and put it on the stack
  }
  assert(B == EVAL_TREE);

  bool matched = false;
  size_t new_root = root;
  do {
    size_t C = B + 1;
    STATE_GET_CELL(C)
    STATE_DEREF(C)
    size_t D = C + 1;
    STATE_GET_CELL(D)
    size_t E = C + 2;
    STATE_GET_CELL(E)
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
    // eval_encode_dump(state->cells, A);
    const size_t REF_SIZE = 1;
    size_t C = B + 1;
    STATE_GET_CELL(C)
    STATE_DEREF(C)
    size_t D = C + 1;
    STATE_GET_CELL(D)
    EXPECT(
        _eval_is_ref(state, D),
        ERROR_REF_EXPECTED,
        "currently cannot get to other sibling if not ref :(");
    size_t E = D + REF_SIZE;
    STATE_GET_CELL(E)
    STATE_DEREF(D);
    if (!(is_opaque(state, D) && E_cell == EVAL_NIL)) {
      break;
    }

#define CALCULATE_OFFSET(from, to)                                                                 \
  sint to##_##from##_ref = to - from;                                                              \
  {                                                                                                \
    size_t tmp = from + to##_##from##_ref;                                                         \
    if (_eval_is_ref(state, tmp)) {                                                                \
      STATE_GET_CELL(tmp)                                                                          \
      STATE_DEREF(tmp)                                                                             \
      to##_##from##_ref = (sint)tmp - (sint)from;                                                  \
    }                                                                                              \
  }

    // NOTE: we could allocate whole 7 cells here and reduce dereference need
    // but for now I stick to reference-based approach
    size_t F = E + 1;
    EXPECT(
        _eval_is_ref(state, F),
        ERROR_REF_EXPECTED,
        "currently cannot get to other sibling if not ref :(");
    size_t G = F + REF_SIZE;
    size_t Q = next_n_vacant_cells(state, 3);
    size_t R = Q + 1;
    size_t S = R + 1;
    CALCULATE_OFFSET(R, D)
    CALCULATE_OFFSET(S, G)

    // TODO: also need to put these (x y) and (y z) on the stack and actually evaluate first

    STATE_SET_CELL(Q, EVAL_APPLY);
    STATE_SET_CELL(R, EVAL_REF);
    STATE_SET_CELL(S, EVAL_REF);
    STATE_SET_WORD(R, EVAL_TAG_INDEX, D_R_ref);
    STATE_SET_WORD(S, EVAL_TAG_INDEX, G_S_ref);

    size_t T = next_n_vacant_cells(state, 3);
    size_t U = T + 1;
    size_t V = U + 1;
    CALCULATE_OFFSET(U, F)
    CALCULATE_OFFSET(V, G)

    STATE_SET_CELL(T, EVAL_APPLY);
    STATE_SET_CELL(U, EVAL_REF);
    STATE_SET_CELL(V, EVAL_REF);
    STATE_SET_WORD(U, EVAL_TAG_INDEX, F_U_ref);
    STATE_SET_WORD(V, EVAL_TAG_INDEX, G_V_ref);

    size_t P = next_n_vacant_cells(state, 3);
    sint Q_P_1_ref = (sint)Q - (sint)(P + 1);
    sint T_P_2_ref = (sint)T - (sint)(P + 2);

    STATE_SET_CELL(P, EVAL_APPLY);
    STATE_SET_CELL(P + 1, EVAL_REF);
    STATE_SET_CELL(P + 2, EVAL_REF);
    STATE_SET_WORD(P + 1, EVAL_TAG_INDEX, Q_P_1_ref);
    STATE_SET_WORD(P + 2, EVAL_TAG_INDEX, T_P_2_ref);
    new_root = P;
    matched = true;
#undef CALCULATE_OFFSET

  } while (0);
  CHECK(state)
  if (matched) {
    result.result = new_root;
    goto cleanup;
  }
  matched = false;

cleanup:
  return result;
}

sint eval_step(EvalState state) {
  return eval_step_impl(state).result;
}

Allocator eval_get_memory(EvalState state) {
  return state->cells;
}

uint8_t eval_get_error(EvalState state, const char** message) {
  *message = state->error;
  return state->error_code;
}
