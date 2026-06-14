

#include "bytecode_api.h"
#include "cells_api.h"
#include "domain_api.h"
#include "reducer_api.h"
#include "reducer_impl.h"
#include "vendor/stb_ds.h"

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define ERR_CHECK(msg)                                                                             \
  do {                                                                                             \
    if ((err) != ERROR_SUCCESS) {                                                                  \
      stbds_arr_printf(&(reducer)->_error, "[ERROR] " msg " %s %d\n", __FILE__, __LINE__);         \
      return (err);                                                                                \
    }                                                                                              \
  } while (0)

#define ERR_CHECK_MASKED(msg, mask)                                                                \
  do {                                                                                             \
    if ((err) != ERROR_SUCCESS) {                                                                  \
      stbds_arr_printf(&(reducer)->_error, "[ERROR] " msg " %s %d\n", __FILE__, __LINE__);         \
      return (mask);                                                                               \
    }                                                                                              \
  } while (0)

typedef struct cells_node_t cells_node_t;

static int stbds_arr_printf(char** arr, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int used = (int)stbds_arrlen(*arr);
  int need = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  if (need < 0) { return -1; }
  stbds_arrsetcap(*arr, used + need + 1);
  va_start(ap, fmt);
  vsnprintf(*arr + used, need + 1, fmt, ap);
  va_end(ap);
  stbds_arrsetlen(*arr, used + need);
  return need;
}

error_t reducer_create(struct reducer_t** reducer, struct cells_t* cells) {
  *reducer = calloc(1, sizeof(reducer_t));
  reducer_reset(*reducer);
  (*reducer)->cells = cells;
  return ERROR_SUCCESS;
}

void reducer_free(struct reducer_t** reducer) {
  stbds_arrfree((*reducer)->stack);
  stbds_arrfree((*reducer)->_stash);
  stbds_arrfree((*reducer)->_error);
  free(*reducer);
}

void reducer_reset(struct reducer_t* reducer) {
  stbds_arrsetlen(reducer->stack, 0);
  reducer->result = 0;
  reducer->has_result = false;
  stbds_arrsetlen(reducer->_stash, 0);
  stbds_arrsetlen(reducer->_error, 0);
}

error_t reducer_step(struct reducer_t* reducer) {
  error_t err = ERROR_SUCCESS;
  if (stbds_arrlenu(reducer->stack) == 0) { return REDUCER_DONE; }

  bool found_apply = false;
  while (stbds_arrlenu(reducer->stack) > 0) {
    size_t i = stbds_arrpop(reducer->stack);
    if (i == REDUCER_APPLY_TOKEN) {
      found_apply = true;
      break;
    }
    stbds_arrput(reducer->_stash, i);
  }

  if (!found_apply) {
    if (stbds_arrlenu(reducer->_stash) != 1) {
      stbds_arr_printf(
          &reducer->_error,
          "[ERROR] Expected single result item after reduction, found %zu\n",
          stbds_arrlenu(reducer->_stash));
      return ERROR_GENERIC;
    }
    reducer->result = stbds_arrpop(reducer->_stash);
    reducer->has_result = true;
    return REDUCER_DONE;
  }

  if (stbds_arrlenu(reducer->_stash) < 2) {
    stbds_arr_printf(
        &reducer->_error,
        "[ERROR] Reducer stash should contain at least two trees to apply, found %zu\n",
        stbds_arrlenu(reducer->_stash));
    return ERROR_GENERIC;
  }

  size_t redex_i = stbds_arrpop(reducer->_stash);
  size_t arg_i = stbds_arrpop(reducer->_stash);

  cells_node_t redex;
  err = cells_dereference_node(reducer->cells, &redex_i, &redex);
  ERR_CHECK_MASKED("", ERROR_INTERNAL);

  switch (cells_node_type_get_arity(redex.header.type)) {
    // rule 0.a
    case 0: {
      cells_node_t arity1 = redex;
      if (!cells_node_set_arity(&arity1, 1)) { return ERROR_INTERNAL; }
      size_t total_size = arity1.header.encoded_size;
      size_t result_index = 0;
      err = cells_alloc_chunk_with_refs(
          reducer->cells,
          total_size,
          (struct opt_size_t){.has_value = true, .value = arg_i},
          (struct opt_size_t){0},
          &result_index);
      ERR_CHECK_MASKED("", ERROR_INTERNAL);
      size_t index_out = result_index + arity1.header.encoded_size;
      i64 arg_shift = arg_i - index_out;
      cells_node_t arg_ref = cells_new_ref(arg_shift);
      if (arg_ref.header.type.value == CELLS_NODE_TYPE_INVALID) { return ERROR_INTERNAL; }
      cells_write_node(reducer->cells, result_index, arity1);
      cells_write_node(reducer->cells, index_out, arg_ref);
      reducer_push_to_stack(reducer, result_index);
      return ERROR_SUCCESS;
    }
    // rule 0.b
    case 1: {
      cells_node_t arity2 = redex;
      if (!cells_node_set_arity(&arity2, 2)) { return ERROR_INTERNAL; }
      size_t delta1_left_i = redex_i;
      cells_node_t delta1_left;
      err = cells_get_left_node(reducer->cells, &delta1_left_i, &delta1_left);
      ERR_CHECK("");
      size_t total_size = arity2.header.encoded_size;
      size_t result_index = 0;
      err = cells_alloc_chunk_with_refs(
          reducer->cells,
          total_size,
          (struct opt_size_t){.has_value = true, .value = delta1_left_i},
          (struct opt_size_t){.has_value = true, .value = arg_i},
          &result_index);
      ERR_CHECK_MASKED("", ERROR_INTERNAL);
      cells_write_node(reducer->cells, result_index, arity2);

      size_t index_out = result_index;
      index_out += arity2.header.encoded_size;
      i64 delta1_shift = delta1_left_i - index_out;
      cells_node_t delta1_ref = cells_new_ref(delta1_shift);
      if (delta1_ref.header.type.value == CELLS_NODE_TYPE_INVALID) { return ERROR_INTERNAL; }
      cells_write_node(reducer->cells, index_out, delta1_ref);

      index_out += delta1_ref.header.encoded_size;
      i64 arg_shift = arg_i - index_out;
      cells_node_t arg_ref = cells_new_ref(arg_shift);
      if (arg_ref.header.type.value == CELLS_NODE_TYPE_INVALID) { return ERROR_INTERNAL; }
      cells_write_node(reducer->cells, index_out, arg_ref);
      reducer_push_to_stack(reducer, result_index);
      return ERROR_SUCCESS;
    }
    // rule 1,2,3
    case 2: {
      cells_node_t redex_left;
      size_t redex_left_i = redex_i;
      err = cells_get_left_node(reducer->cells, &redex_left_i, &redex_left);
      ERR_CHECK("");
      cells_node_t redex_right;
      size_t redex_right_i = redex_i;
      err = cells_get_right_node(reducer->cells, &redex_right_i, &redex_right);
      ERR_CHECK("");

      switch (cells_node_type_get_arity(redex_left.header.type)) {
        case 0: {
          // rule 1
          reducer_push_to_stack(reducer, redex_right_i);
          return ERROR_SUCCESS;
        }
        case 1: {
          // rule 2
          cells_node_t x;
          size_t x_i = redex_left_i;
          err = cells_get_left_node(reducer->cells, &x_i, &x);
          ERR_CHECK("");

          size_t y_i = redex_right_i;

          cells_node_t z;
          size_t z_i = arg_i;
          err = cells_dereference_node(reducer->cells, &z_i, &z);
          ERR_CHECK("");

          reducer_push_to_stack(reducer, REDUCER_APPLY_TOKEN);
          reducer_push_to_stack(reducer, REDUCER_APPLY_TOKEN);
          reducer_push_to_stack(reducer, x_i);
          reducer_push_to_stack(reducer, z_i);
          reducer_push_to_stack(reducer, REDUCER_APPLY_TOKEN);
          reducer_push_to_stack(reducer, y_i);
          reducer_push_to_stack(reducer, z_i);
          return ERROR_SUCCESS;
        }
        case 2: {
          // rule 3
          cells_node_t w;
          size_t w_i = redex_left_i;
          err = cells_get_left_node(reducer->cells, &w_i, &w);
          ERR_CHECK("");

          cells_node_t x;
          size_t x_i = redex_left_i;
          err = cells_get_right_node(reducer->cells, &x_i, &x);
          ERR_CHECK("");

          size_t y_i = redex_right_i;

          cells_node_t z;
          size_t z_i = arg_i;
          err = cells_dereference_node(reducer->cells, &z_i, &z);
          ERR_CHECK("");

          switch (cells_node_type_get_arity(z.header.type)) {
            case 0: {
              // rule 3a
              reducer_push_to_stack(reducer, w_i);
              return ERROR_SUCCESS;
            }
            case 1: {
              // rule 3b
              cells_node_t u;
              size_t u_i = z_i;
              err = cells_get_left_node(reducer->cells, &u_i, &u);
              ERR_CHECK("");

              reducer_push_to_stack(reducer, REDUCER_APPLY_TOKEN);
              reducer_push_to_stack(reducer, x_i);
              reducer_push_to_stack(reducer, u_i);
              return ERROR_SUCCESS;
            }
            case 2: {
              // rule 3c
              cells_node_t u;
              size_t u_i = z_i;
              err = cells_get_left_node(reducer->cells, &u_i, &u);
              ERR_CHECK("");

              cells_node_t v;
              size_t v_i = z_i;
              err = cells_get_right_node(reducer->cells, &v_i, &v);
              ERR_CHECK("");

              reducer_push_to_stack(reducer, REDUCER_APPLY_TOKEN);
              reducer_push_to_stack(reducer, REDUCER_APPLY_TOKEN);
              reducer_push_to_stack(reducer, y_i);
              reducer_push_to_stack(reducer, u_i);
              reducer_push_to_stack(reducer, v_i);
              return ERROR_SUCCESS;
            }
            default:
              stbds_arr_printf(&reducer->_error, "[ERROR] Unexpected node type at %zu", z_i);
              return ERROR_GENERIC;
          }

          return ERROR_SUCCESS;
        }
        default:
          stbds_arr_printf(&reducer->_error, "[ERROR] Unexpected node type at %zu", redex_left_i);
          return ERROR_GENERIC;
      }
    }
    default:
      stbds_arr_printf(&reducer->_error, "[ERROR] Unexpected node type at %zu", redex_i);
      return ERROR_GENERIC;
  }

  return ERROR_SUCCESS;
}

const char* reducer_get_error(struct reducer_t* reducer) {
  return reducer->_error;
}

void reducer_push_to_stack(struct reducer_t* reducer, size_t index) {
  stbds_arrpush(reducer->stack, index);
}

bool reducer_has_result(struct reducer_t* reducer) {
  return reducer->has_result;
}

size_t reducer_get_result(struct reducer_t* reducer) {
  assert(reducer->has_result);
  return reducer->result;
}
