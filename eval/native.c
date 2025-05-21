#include "api.h"
#include <stdio.h>

#include "eval.h"
#include "native.h"
#include "util.h"

sint native_load_standard(eval_state_t* state) {
  sint err = 0;
  // ^ T [integer]
  err = eval_add_native(state, "type.integer", NATIVE_TYPE_INTEGER);
  CHECK_ERROR({})
  // ^ T [ref] -> ^ a ^ b ... ^ z *
  err = eval_add_native(state, "type.list", NATIVE_TYPE_LIST);
  CHECK_ERROR({})

  err = eval_add_native(state, "io.print", (uint)_native_io_print);
  CHECK_ERROR({})
error:
  return err;
}

static inline bool _check_tag(eval_state_t* state, size_t value, uint tag) {
  size_t cell = _eval_get_left_node(state, value);
  if (cell == value) {
    return false;
  }
  sint word = 0;
  sint err = eval_cells_get_word(state->cells, cell, &word);
  if (err == ERR_VAL) {
    return false;
  }
  return (uint)word == tag;
}

static bool _native_is_integer(eval_state_t* state, size_t value) {
  return _check_tag(state, value, NATIVE_TYPE_INTEGER);
}

static bool _native_is_list(eval_state_t* state, size_t value) {
  return _check_tag(state, value, NATIVE_TYPE_LIST);
}

static sint _native_as_integer(eval_state_t* state, size_t value) {
  size_t cell = _eval_get_right_node(state, value);
  EVAL_ASSERT(cell != value, ERROR_INVALID_CAST, "");
  sint word = 0;
  sint err = eval_cells_get_word(state->cells, cell, &word);
  EVAL_ASSERT(err != ERR_VAL, ERROR_GENERIC, "")
  return word;
error:
  return -1;
}

size_t _native_io_print(eval_state_t* state, size_t arg) {
  if (_native_is_integer(state, arg)) {
    sint n = _native_as_integer(state, arg);
    EVAL_CHECK_STATE(state)
    // TODO: no unicode for now :(
    printf("%c", (char)n);
    goto success;
  }

  if (_native_is_list(state, arg)) {
    size_t payload = _eval_get_right_node(state, arg);
    payload = _eval_dereference(state, payload);
    sint payload_cell = eval_cells_get(state->cells, payload);
    EVAL_ASSERT(payload_cell != ERR_VAL, ERROR_INVALID_TREE, "")
    while (!_eval_is_nil(payload_cell)) {
      size_t value = _eval_get_left_node(state, payload);
      EVAL_ASSERT(value != payload, ERROR_GENERIC, "");
      (void)_native_io_print(state, value);
      EVAL_CHECK_STATE(state)
      size_t next = _eval_get_right_node(state, payload);
      EVAL_ASSERT(next != payload, ERROR_GENERIC, "");

      payload = _eval_dereference(state, next);
      payload_cell = eval_cells_get(state->cells, payload);
      EVAL_ASSERT(payload_cell != ERR_VAL, ERROR_INVALID_TREE, "")
    }
  }
error:
success:
  return arg;
}
