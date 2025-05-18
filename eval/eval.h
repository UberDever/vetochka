#ifndef __EVAL_EVAL__
#define __EVAL_EVAL__

#include "api.h"
#include <stdbool.h>

#define SIGIL_NIL  0
#define SIGIL_TREE 1
#define SIGIL_REF  2

#define TOKEN_APPLY SIZE_MAX

struct json_parser_t;

typedef struct {
  const char* key;
  native_symbol_t value;
} native_entry_t;

struct eval_state_t {
  allocator_t* cells;
  size_t* apply_stack;
  size_t* result_stack;

  u64* free_bitmap;
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

#endif
