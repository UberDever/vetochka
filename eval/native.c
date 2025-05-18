#include "api.h"
#include "memory.h"
#include <stdio.h>

#include "eval.h"
#include "native.h"
#include "util.h"

static bool _native_is_number(eval_state_t* state, size_t value) {
  // TODO: need error checking here
  size_t cell = _get_left_node(state, value);
  sint word = eval_cells_get_word(state->cells, cell);
  u8 tag = _tv_get_tag(word);
  i64 payload = _tv_get_payload_signed(word);
  return tag == WORD_TAG_NUMBER && payload == VALUE_TYPE_NUMBER;
}

static sint _native_as_number(eval_state_t* state, size_t value) {
  // TODO: need error checking here
  size_t cell = _get_right_node(state, value);
  sint word = eval_cells_get_word(state->cells, cell);
  return _tv_get_payload_signed(word);
}

size_t _native_io_println(eval_state_t* state, size_t arg) {
  // TODO: need error checking here
  if (_native_is_number(state, arg)) {
    sint n = _native_as_number(state, arg);
    logg("Got a number %ld", n);
  }
  return arg;
}
