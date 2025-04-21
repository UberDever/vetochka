

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "vendor/json.h"
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
#define ERROR_GENERIC         127

struct EvalState_impl {
  Allocator cells;
  Stack control_stack;
  size_t* value_stack;
  u64* free_bitmap;
  size_t free_capacity;

  uint8_t error_code;
  const char* error;
};

sint eval_init_from_program(EvalState* state, const char* program) {
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
  stbds_arrput(
      s->control_stack, ((struct StackEntry){.type = StackEntryType_Index, .as_index = 0}));
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
  stbds_arrput(
      s->control_stack, ((struct StackEntry){.type = StackEntryType_Index, .as_index = 0}));
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
  stbds_arrfree(s->control_stack);
  stbds_arrfree(s->value_stack);
  free(s->free_bitmap);
  free(s);
  *state = NULL;
  return 0;
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

#define STATE_SET_REF(index, ref)                                                                  \
  do {                                                                                             \
    sint res = eval_cells_set_word(state->cells, index, ref);                                      \
    EXPECT(res != ERR_VAL, ERROR_GENERIC, "");                                                     \
  } while (0)

#if 0
#define STATE_SET_WORD(index, tag, payload)                                                        \
  do {                                                                                             \
    sint word = 0;                                                                                 \
    word = eval_tv_set_tag(word, tag);                                                             \
    word = eval_tv_set_payload_signed(word, payload);                                              \
    sint res = eval_cells_set_word(state->cells, index, word);                                     \
    EXPECT(res != ERR_VAL, ERROR_GENERIC, "");                                                     \
  } while (0)
#endif

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

// clang-format off
// Namings: A   B   C   D   E   F   G   H       P   Q   R   S   T   U   V
// Rule 1 : ^   ^   *   *   X   Y           ->  X                         , new roots: P
// Rule 2 : ^   ^   X   *   Y   Z           ->  X   Z   Y   Z             , new roots: eval P, eval R, eval P $ R
// Rule 3a: ^   ^   W   X   Y   ^   *   *   ->  W                         , new roots: P
// Rule 3b: ^   ^   W   X   Y   ^   U   *   ->  X   U                     , new roots: P
// Rule 3c: ^   ^   W   X   Y   ^   U   V   ->  Y   U   V                 , new roots: eval P, eval P $ R
// Rule 4 : $   N   X, where N is native function and X is a arbitrary value
// clang-format on
void eval_step(EvalState state) {
  EXPECT(stbds_arrlenu(state->control_stack) > 0, ERROR_STACK_UNDERFLOW, "");

  // TODO: add data (fully evaluated) as stack entry
  // skip it in simple evaluation until calculated root is found (eval it) or until simple index
  // is found (eval it and insert)
  struct StackEntry root = stbds_arrpop(state->control_stack);
  if (root.type == StackEntryType_Calculated) {
    if (root.as_calculated_index.type == CalculatedIndexType_Rule2) {
      EXPECT(
          stbds_arrlenu(state->control_stack) >= 2,
          ERROR_STACK_UNDERFLOW,
          "stack underflow in calculated index rule2");

      goto matched;
    }
    if (root.as_calculated_index.type == CalculatedIndexType_Rule3c) {
      goto matched;
    }
    assert(false);
  }
  assert(root.type == StackEntryType_Index);

  size_t A = /*A + 1*/ root.as_index;
  STATE_GET_CELL(A)
  STATE_DEREF(A)

  EXPECT(A_cell != EVAL_NIL, ERROR_GENERIC, "")
  /*if (B == EVAL_NATIVE) {
    STATE_GET_WORD(B)
    u8 tag = eval_tv_get_tag(B_word);
    EXPECT(tag == EVAL_TAG_FUNC, ERROR_APPLY_TO_VALUE, "")
    assert(false); // TODO: this
  } else */
  /*if (B_cell == EVAL_APPLY) {
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
  }*/
  assert(A_cell == EVAL_TREE);

  bool matched = false;
  do {
    size_t B = A + 1;
    STATE_GET_CELL(B)
    STATE_DEREF(B)
    size_t C = B + 1;
    STATE_GET_CELL(C)
    size_t D = B + 2;
    STATE_GET_CELL(D)
    size_t E = D + 1;
    if (!_eval_is_ref(state, E)) {
      break;
    }
    size_t F = E + 1;
    if (!_eval_is_ref(state, F)) {
      break;
    }
    if (!(C_cell == EVAL_NIL && D_cell == EVAL_NIL)) {
      break;
    }

    stbds_arrput(
        state->control_stack, ((struct StackEntry){.type = StackEntryType_Index, .as_index = E}));
    matched = true;
  } while (0);
  CHECK(state)
  if (matched) {
    goto matched;
  }
  matched = false;

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

  do {
    // eval_encode_dump(state->cells, A);
    // const size_t REF_SIZE = 1;
    size_t B = A + 1;
    STATE_GET_CELL(B)
    STATE_DEREF(B)
    size_t C = B + 1;
    STATE_GET_CELL(C)
    if (!_eval_is_ref(state, C)) {
      break;
    }
    size_t D = C + 1;
    STATE_GET_CELL(D)
    STATE_DEREF(C);
    size_t E = D + 1;
    if (!_eval_is_ref(state, E)) {
      break;
    }
    size_t F = E + 1;
    if (!_eval_is_ref(state, F)) {
      break;
    }
    if (!(is_opaque(state, C) && D_cell == EVAL_NIL)) {
      break;
    }

    size_t P = next_n_vacant_cells(state, 4);
    size_t Q = P + 1;
    CALCULATE_OFFSET(P, C)
    CALCULATE_OFFSET(Q, F)

    STATE_SET_CELL(P, EVAL_REF);
    STATE_SET_REF(P, C_P_ref);
    STATE_SET_CELL(Q, EVAL_REF);
    STATE_SET_REF(Q, F_Q_ref);

    size_t R = P + 2;
    size_t S = P + 3;
    CALCULATE_OFFSET(R, E)
    CALCULATE_OFFSET(S, F)

    STATE_SET_CELL(R, EVAL_REF);
    STATE_SET_REF(R, E_R_ref);
    STATE_SET_CELL(S, EVAL_REF);
    STATE_SET_REF(S, F_S_ref);

    stbds_arrput(
        state->control_stack, ((struct StackEntry){.type = StackEntryType_Index, .as_index = P}));
    stbds_arrput(
        state->control_stack, ((struct StackEntry){.type = StackEntryType_Index, .as_index = R}));
    stbds_arrput(
        state->control_stack,
        ((struct StackEntry){
            .type = StackEntryType_Calculated,
            .as_calculated_index = {.type = CalculatedIndexType_Rule2}}));

    matched = true;

  } while (0);
#undef CALCULATE_OFFSET

  CHECK(state)
  if (matched) {
    goto matched;
  }
  matched = false;

  stbds_arrpush(state->control_stack, root);

  return;

cleanup:
matched:
  // NOTE: if after calculating the specified root from the stack
  // we got an entry in the control stack
  // then we (1) pop that entry and do the following:
  // if entry was rule2 -> P' = stack_pop; R = stack_pop; stack_push P'; stack_push R
  // if entry was rule3c -> P' = stack_pop; ...?

  return;
}

#undef EXPECT
#undef CHECK
#undef STATE_GET_CELL
#undef STATE_SET_CELL
#undef STATE_GET_WORD
#undef STATE_SET_REF
#undef STATE_DEREF

Allocator eval_get_memory(EvalState state) {
  return state->cells;
}

Stack eval_get_stack(EvalState state) {
  return state->control_stack;
}

uint8_t eval_get_error(EvalState state, const char** message) {
  *message = state->error;
  return state->error_code;
}

#define CHECK(cond)                                                                                \
  if (!(cond)) {                                                                                   \
    goto cleanup;                                                                                  \
  }

static sint dump_control_stack(StringBuffer json_out, Stack stack) {
  sint result = 0;
  const char* mappings[] = {"INVALID", "rule2", "rule3c"};
  _sb_printf(json_out, "\"control_stack\": [");
  for (size_t i = 0; i < stbds_arrlenu(stack); ++i) {
    struct StackEntry e = stack[i];
    if (e.type == StackEntryType_Index) {
      _sb_printf(json_out, "%d, ", e.as_index);
    } else if (e.type == StackEntryType_Calculated) {
      _sb_printf(
          json_out,
          "{ \"compute_index\": \"%s\" }, ",
          mappings[(size_t)e.as_calculated_index.type]);
    } else {
      assert(false);
    }
  }
  _sb_try_chop_suffix(json_out, ", ");
  _sb_append_str(json_out, "]");

  return result;
}

static sint dump_value_stack(StringBuffer json_out, size_t* stack) {
  sint result = 0;
  _sb_printf(json_out, "\"value_stack\": [");
  for (size_t i = 0; i < stbds_arrlenu(stack); ++i) {
    _sb_printf(json_out, "%d, ", stack[i]);
  }
  _sb_try_chop_suffix(json_out, ", ");
  _sb_append_str(json_out, "]");
  return result;
}

sint eval_dump_json(StringBuffer json_out, EvalState state) {
  sint result = 0;
  _sb_append_str(json_out, "{\n");

  result = eval_cells_dump_json(json_out, state->cells);
  CHECK(result == 0);
  _sb_append_str(json_out, ",\n");

  result = dump_control_stack(json_out, state->control_stack);
  CHECK(result == 0);
  _sb_append_str(json_out, ",\n");

  result = dump_value_stack(json_out, state->value_stack);
  CHECK(result == 0);
  _sb_append_str(json_out, ",\n");

  if (_sb_try_chop_suffix(json_out, ",\n")) {
    _sb_append_char(json_out, '\n');
  }
  _sb_append_char(json_out, '}');
cleanup:
  return result;
}

sint eval_load_json(const char* json, EvalState* state) {
  sint err = eval_init(state);
  if (err == ERR_VAL) {
    printf("err %ld", err);
    return err;
  }

  struct json_parse_result_s result = {};
  struct json_value_s* root = json_parse_ex(json, strlen(json), 0, NULL, NULL, &result);
  if (result.error != json_parse_error_none) {
    printf("error while parsing json at %zu %zu\n", result.error_line_no, result.error_row_no);
    return result.error;
  }
  struct json_object_s* object = json_value_as_object(root);

  err = eval_cells_load_json(object, &(*state)->cells);
  if (err) {
    goto cleanup;
  }

cleanup:
  free(root);
  return 0;
}

#undef CHECK
