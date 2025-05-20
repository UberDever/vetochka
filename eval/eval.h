#ifndef __EVAL_EVAL__
#define __EVAL_EVAL__

#include "api.h"
#include <stdbool.h>

#define SIGIL_NIL  0
#define SIGIL_TREE 1
#define SIGIL_REF  2

#define TOKEN_APPLY SIZE_MAX

#define ERROR_PARSE           1
#define ERROR_STACK_UNDERFLOW 2
#define ERROR_APPLY_TO_VALUE  3
#define ERROR_INVALID_TREE    4
#define ERROR_INVALID_CAST    5
#define ERROR_GENERIC         127

#define EVAL_ASSERT(cond, code, msg)                                                               \
  if (!(cond)) {                                                                                   \
    state->error_code = (code);                                                                    \
    _errbuf_write("%s %s %s\n", #cond, #code, msg);                                                \
    goto error;                                                                                    \
  }

#define EVAL_CHECK_STATE(state)                                                                    \
  if (state->error_code) {                                                                         \
    goto error;                                                                                    \
  }

struct json_parser_t;

typedef struct {
  const char* key;
  uint value;
} native_entry_t;

struct eval_state_t {
  allocator_t* cells;
  size_t* apply_stack;
  size_t* result_stack;

  uint* free_bitmap;
  size_t free_capacity;
  u8* match_stack;

  native_entry_t* native_symbols;
  uint8_t error_code;
  const char* error;
};

sint _eval_reset_cells(eval_state_t* state);
void _eval_result_stack_push(eval_state_t* state, size_t value);

static inline bool _is_leaf(sint root, sint left, sint right) {
  return root == SIGIL_TREE && left == SIGIL_NIL && right == SIGIL_NIL;
}

static inline bool _is_ref(sint root, sint left, sint right) {
  return root == SIGIL_REF && left == SIGIL_NIL && right == SIGIL_NIL;
}

static inline bool _is_native(sint root, sint left, sint right) {
  return root == SIGIL_REF && left == SIGIL_REF && right == SIGIL_NIL;
}

bool _is_terminal(eval_state_t* state, size_t index);
size_t _get_left_node(eval_state_t* state, size_t root_index);
size_t _get_right_node(eval_state_t* state, size_t root_index);
void _errbuf_write(const char* format, ...);
void _errbuf_clear();

#endif
