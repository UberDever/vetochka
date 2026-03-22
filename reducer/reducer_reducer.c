

#include "cells_api.h"
#include "reducer_api.h"
#include "reducer_impl.h"
#include "typedefs.h"
#include "vendor/stb_ds.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct cells_node_t cells_node_t;
typedef struct cells_node_meta_t cells_node_meta_t;

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

  if (stbds_arrlenu(reducer->stack) < 2) {
    stbds_arr_printf(&reducer->_error, "[ERROR] Reducer stack underflow\n");
    return ERROR_GENERIC;
  }
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

  if (stbds_arrlenu(reducer->_stash) != 2) {
    stbds_arr_printf(
        &reducer->_error,
        "[ERROR] Reducer stash should contain two trees to apply, found %zu\n",
        stbds_arrlenu(reducer->_stash));
    return ERROR_GENERIC;
  }

  size_t redex_i = stbds_arrpop(reducer->_stash);
  size_t arg_i = stbds_arrpop(reducer->_stash);

  cells_node_t redex;
  err = cells_dereference_node(reducer->cells, &redex_i, &redex);
  if (err != ERROR_SUCCESS) {
    stbds_arr_printf(&reducer->_error, "[ERROR] Cannot get redex node %s %d\n", __FILE__, __LINE__);
    return ERROR_INTERNAL;
  }
  // cells_node_t arg = dereference_node(reducer, &arg_i);
  if (reducer->_error != NULL) {
    stbds_arr_printf(&reducer->_error, "[ERROR] Cannot get arg node %s %d\n", __FILE__, __LINE__);
    return ERROR_GENERIC;
  }

  switch (redex.meta.type) {
    // rule 0.a
    case CELLS_NODE_TYPE_DELTA0: {
      cells_node_t delta1 = cells_new_delta1();
      size_t total_size = delta1.meta.size;
      size_t result_index = 0;
      err = cells_alloc_chunk_with_refs(
          reducer->cells,
          total_size,
          (struct opt_size_t){.has_value = true, .value = arg_i},
          (struct opt_size_t){0},
          &result_index);
      if (err != ERROR_SUCCESS) {
        stbds_arr_printf(&reducer->_error, "[ERROR] %s %d\n", __FILE__, __LINE__);
        return ERROR_INTERNAL;
      }
      size_t index_out = result_index + delta1.meta.size;
      i64 arg_shift = arg_i - index_out;
      cells_node_t arg_ref;
      if (cells_fits_in_ref2(arg_shift)) {
        arg_ref = cells_new_ref2(arg_shift);
      } else {
        assert(cells_fits_in_ref8(arg_shift));
        arg_ref = cells_new_ref8(arg_shift);
      }
      cells_write_node(reducer->cells, result_index, delta1);
      cells_write_node(reducer->cells, index_out, arg_ref);
      reducer->result = result_index;
      reducer->has_result = true;
      return REDUCER_DONE;
    }
    // rule 0.b
    case CELLS_NODE_TYPE_DELTA1: {
      cells_node_t delta2 = cells_new_delta2();
      size_t delta1_left_i = redex_i;
      cells_node_t delta1_left;
      err = cells_get_left_node(reducer->cells, &delta1_left_i, &delta1_left);
      if (err != ERROR_SUCCESS) {
        stbds_arr_printf(&reducer->_error, "[ERROR] %s %d\n", __FILE__, __LINE__);
        return err;
      }
      size_t total_size = delta2.meta.size;
      size_t result_index = 0;
      err = cells_alloc_chunk_with_refs(
          reducer->cells,
          total_size,
          (struct opt_size_t){.has_value = true, .value = delta1_left_i},
          (struct opt_size_t){.has_value = true, .value = arg_i},
          &result_index);
      if (err != ERROR_SUCCESS) {
        stbds_arr_printf(&reducer->_error, "[ERROR] %s %d\n", __FILE__, __LINE__);
        return ERROR_INTERNAL;
      }
      cells_write_node(reducer->cells, result_index, delta2);

      size_t index_out = result_index;
      index_out += delta2.meta.size;
      i64 delta1_shift = delta1_left_i - index_out;
      cells_node_t delta1_ref;
      if (cells_fits_in_ref2(delta1_shift)) {
        delta1_ref = cells_new_ref2(delta1_shift);
      } else {
        assert(cells_fits_in_ref8(delta1_shift));
        delta1_ref = cells_new_ref8(delta1_shift);
      }
      cells_write_node(reducer->cells, index_out, delta1_ref);

      index_out += delta1_ref.meta.size;
      i64 arg_shift = arg_i - index_out;
      cells_node_t arg_ref;
      if (cells_fits_in_ref2(arg_shift)) {
        arg_ref = cells_new_ref2(arg_shift);
      } else {
        assert(cells_fits_in_ref8(arg_shift));
        arg_ref = cells_new_ref8(arg_shift);
      }
      cells_write_node(reducer->cells, index_out, arg_ref);
      reducer->result = result_index;
      reducer->has_result = true;
      return REDUCER_DONE;
    }
    // rule 1,2,3
    case CELLS_NODE_TYPE_DELTA2:
    default:
      stbds_arr_printf(&reducer->_error, "[ERROR] The node at %zu is not applicable", redex_i);
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
