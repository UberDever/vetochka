#include "api.h"
#include <stdio.h>

#include "eval.h"
#include "native.h"
#include "util.h"

sint native_load_standard(eval_state_t* state) {
  sint err = 0;
  err = eval_add_native(state, "type.number", NATIVE_TYPE_NUMBER);
  CHECK_ERROR({})
  err = eval_add_native(state, "io.println", (uint)_native_io_println);
  CHECK_ERROR({})
error:
  return err;
}

static bool _native_is_number(eval_state_t* state, size_t value) {
  size_t cell = _get_left_node(state, value);
  if (cell == value) {
    return false;
  }
  sint word = 0;
  sint err = eval_cells_get_word(state->cells, cell, &word);
  if (err == ERR_VAL) {
    return false;
  }
  return word == NATIVE_TYPE_NUMBER;
}

static sint _native_as_number(eval_state_t* state, size_t value) {
  size_t cell = _get_right_node(state, value);
  EVAL_ASSERT(cell != value, ERROR_INVALID_CAST, "");
  sint word = 0;
  sint err = eval_cells_get_word(state->cells, cell, &word);
  EVAL_ASSERT(err != ERR_VAL, ERROR_GENERIC, "")
  return word;
error:
  return -1;
}

size_t _native_io_println(eval_state_t* state, size_t arg) {
  if (_native_is_number(state, arg)) {
    sint n = _native_as_number(state, arg);
    EVAL_CHECK_STATE(state)
    logg("Got a number %ld", n);
  }
error:
  return arg;
}
