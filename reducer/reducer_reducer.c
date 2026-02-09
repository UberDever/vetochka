

#include "cells_api.h"
#include "reducer_api.h"
#include "reducer_impl.h"
#include "typedefs.h"
#include "vendor/stb_ds.h"
#include <assert.h>
#include <stdarg.h>
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
  stbds_arrfree((*reducer)->stash);
  stbds_arrfree((*reducer)->error);
  free(*reducer);
}

void reducer_reset(struct reducer_t* reducer) {
  reducer->cells = NULL;
  stbds_arrsetlen(reducer->stack, 0);
  reducer->result = 0;
  stbds_arrsetlen(reducer->stash, 0);
  stbds_arrsetlen(reducer->error, 0);
}

// NOTE: this allows ref to ref, and doesn't handle cycles
// cycle currently considered as malformed bytecode, so hanging is abnormal behavior
static cells_node_t dereference_node(struct reducer_t* reducer, size_t index) {
  while (1) {
    cells_node_meta_t meta = cells_get_node_meta(reducer->cells, index);
    if (meta.type == CELLS_NODE_TYPE_INVALID) { return (cells_node_t){0}; }
    cells_node_t node = cells_get_node(reducer->cells, index, meta);
    if (node.meta.type == CELLS_NODE_TYPE_INVALID) { return (cells_node_t){0}; }
    if (meta.type == CELLS_NODE_TYPE_REF1 || meta.type == CELLS_NODE_TYPE_REF2
        || meta.type == CELLS_NODE_TYPE_REF4 || meta.type == CELLS_NODE_TYPE_REF8) {
      index += node.as.ref;
      continue;
    }
    return node;
  }
}

// TODO: need to set errors in the reducer state here
static cells_node_t get_left_node(struct reducer_t* reducer, size_t parent_index) {
  size_t index = parent_index + 1;
  return dereference_node(reducer, index);
}

static cells_node_t get_right_node(struct reducer_t* reducer, size_t parent_index) {
  cells_node_meta_t meta = cells_get_node_meta(reducer->cells, parent_index + 1);
  if (meta.type == CELLS_NODE_TYPE_INVALID) { return (cells_node_t){0}; }
  if (!(meta.type == CELLS_NODE_TYPE_REF1 || meta.type == CELLS_NODE_TYPE_REF2
        || meta.type == CELLS_NODE_TYPE_REF4 || meta.type == CELLS_NODE_TYPE_REF8)) {
    return (cells_node_t){0};
  }

  size_t index = parent_index + 2;
  return dereference_node(reducer, index);
}

error_t reducer_step(struct reducer_t* reducer) {
  if (stbds_arrlenu(reducer->stack) == 0) { return REDUCER_DONE; }

  bool found_apply = false;
  while (stbds_arrlenu(reducer->stack) > 0) {
    size_t i = stbds_arrpop(reducer->stack);
    if (i == REDUCER_APPLY_TOKEN) {
      found_apply = true;
      break;
    }
    stbds_arrput(reducer->stash, i);
  }

  if (!found_apply) { return REDUCER_DONE; }

  if (stbds_arrlenu(reducer->stash) < 2) {
    stbds_arr_printf(&reducer->error, "[ERROR] Not enough arguments for apply\n");
    return ERROR_GENERIC;
  }

  size_t func_i = stbds_arrpop(reducer->stash);
  cells_node_t func = dereference_node(reducer, func_i);
  if (reducer->error != NULL) { return ERROR_GENERIC; }
  // size_t arg_i = stbds_arrpop(reducer->stash);
  // cells_node_t arg = dereference_node(reducer, arg_i);
  if (reducer->error != NULL) { return ERROR_GENERIC; }

  if (cells_is_opcode(func.meta)) { assert(0 && "TODO"); }

  switch (func.meta.type) {
    // rule 0.a
    case CELLS_NODE_TYPE_TREE0: {
      // cells_node_t tree1 = cells_new_tree1();
      // cells_node_t ref = cells_new_ref1(0);
      // size_t total_size = tree1.meta.size + ref.meta.size;
      // size_t new_index = 0;

      break;
    }
    // rule 0.b
    case CELLS_NODE_TYPE_TREE1:
    // rule 1,2,3
    case CELLS_NODE_TYPE_TREE2:
    default:
      stbds_arr_printf(&reducer->error, "[ERROR] The node at %zu is not appliable", func_i);
      return ERROR_GENERIC;
  }

  (void)get_left_node;
  (void)get_right_node;
  return ERROR_SUCCESS;
}

const char* reducer_get_error(struct reducer_t* reducer) {
  return reducer->error;
}

void reducer_push_to_stack(struct reducer_t* reducer, size_t index) {
  stbds_arrpush(reducer->stack, index);
}

size_t reducer_get_result(struct reducer_t* reducer) {
  return reducer->result;
}
