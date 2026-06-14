#include "bytecode_api.h"
#include "cells_api.h"
#include "domain_api.h"
#include "vendor/stb_ds.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef struct bytecode_tree_t {
  struct cells_node_t payload;
  struct opt_size_t left;
  struct opt_size_t right;
} cells_tree_t;

typedef struct bytecode_tree_builder_t {
  struct bytecode_tree_t* nodes;
} bytecode_tree_builder_t;

typedef struct cells_node_t cells_node_t;

typedef struct bytecode_allocation_t {
  size_t index;
  size_t size;
} bytecode_allocation_t;

typedef struct bytecode_build_t {
  struct bytecode_tree_builder_t* builder;
  struct cells_t* cells;
  bytecode_allocation_t* allocations;
} bytecode_build_t;

error_t bytecode_tree_builder_create(struct bytecode_tree_builder_t** builder) {
  if (builder == NULL) { return ERROR_INVALID_PARAM; }
  *builder = NULL;
  *builder = calloc(1, sizeof(struct bytecode_tree_builder_t));
  if (!*builder) { return ERROR_NOMEM; }
  return ERROR_SUCCESS;
}

void bytecode_tree_builder_destroy(struct bytecode_tree_builder_t** builder) {
  if (builder == NULL || *builder == NULL) { return; }
  stbds_arrfree((*builder)->nodes);
  free(*builder);
  *builder = NULL;
}

void bytecode_tree_builder_reset(struct bytecode_tree_builder_t* builder) {
  if (builder == NULL) { return; }
  stbds_arrsetlen(builder->nodes, 0);
}

static error_t do_build(bytecode_build_t* build, struct opt_size_t root_i, size_t* index_out) {
  error_t err = ERROR_SUCCESS;
  if (!root_i.has_value) { return ERROR_SUCCESS; }
  struct bytecode_tree_t root = build->builder->nodes[root_i.value];

  size_t left_i = SIZE_MAX;
  err = do_build(build, root.left, &left_i);
  if (err != ERROR_SUCCESS) { return err; }
  size_t right_i = SIZE_MAX;
  err = do_build(build, root.right, &right_i);
  if (err != ERROR_SUCCESS) { return err; }

  if (left_i == SIZE_MAX && right_i == SIZE_MAX) {
    err = cells_alloc_chunk(build->cells, root.payload.header.encoded_size, index_out);
    if (err != ERROR_SUCCESS) { return err; }
    stbds_arrput(
        build->allocations,
        ((bytecode_allocation_t){
            .index = *index_out,
            .size = root.payload.header.encoded_size,
        }));
    return cells_write_node(build->cells, *index_out, root.payload);
  }
  assert(left_i != SIZE_MAX);

  struct opt_size_t left_index = {0};
  struct opt_size_t right_index = {0};
  left_index = (struct opt_size_t){.has_value = true, .value = left_i};
  if (right_i != SIZE_MAX) {
    right_index = (struct opt_size_t){.has_value = true, .value = right_i};
  }

  err = cells_alloc_chunk_with_refs(
      build->cells, root.payload.header.encoded_size, left_index, right_index, index_out);
  if (err != ERROR_SUCCESS) { return err; }

  i64 lhs_ref_i = (i64)left_i - (i64)(*index_out + root.payload.header.encoded_size);
  cells_node_t lhs_ref = cells_new_ref(lhs_ref_i);
  if (lhs_ref.header.type.value == CELLS_NODE_TYPE_INVALID) { return ERROR_OVERFLOW; }
  size_t allocation_size = root.payload.header.encoded_size + lhs_ref.header.encoded_size;
  cells_node_t rhs_ref = {0};
  if (right_i != SIZE_MAX) {
    i64 rhs_ref_i = (i64)right_i - (i64)(*index_out + allocation_size);
    rhs_ref = cells_new_ref(rhs_ref_i);
    if (rhs_ref.header.type.value == CELLS_NODE_TYPE_INVALID) { return ERROR_OVERFLOW; }
    allocation_size += rhs_ref.header.encoded_size;
  }
  stbds_arrput(
      build->allocations,
      ((bytecode_allocation_t){
          .index = *index_out,
          .size = allocation_size,
      }));

  err = cells_write_node(build->cells, *index_out, root.payload);
  if (err != ERROR_SUCCESS) { return err; }
  err = cells_write_node(build->cells, *index_out + root.payload.header.encoded_size, lhs_ref);
  if (err != ERROR_SUCCESS) { return err; }

  if (right_i == SIZE_MAX) { return err; }
  err = cells_write_node(
      build->cells,
      *index_out + root.payload.header.encoded_size + lhs_ref.header.encoded_size,
      rhs_ref);
  if (err != ERROR_SUCCESS) { return err; }

  return err;
}

error_t bytecode_tree_builder_build(
    struct bytecode_tree_builder_t* builder, struct cells_t* cells, size_t* index_out) {
  if (builder == NULL || cells == NULL || index_out == NULL) { return ERROR_INVALID_PARAM; }
  size_t len = stbds_arrlenu(builder->nodes);
  if (len == 0) { return ERROR_SUCCESS; }
  bytecode_build_t build = {
      .builder = builder,
      .cells = cells,
  };
  error_t err =
      do_build(&build, (struct opt_size_t){.has_value = true, .value = len - 1}, index_out);
  if (err != ERROR_SUCCESS) {
    for (size_t i = stbds_arrlenu(build.allocations); i != 0; i--) {
      bytecode_allocation_t allocation = build.allocations[i - 1];
      error_t free_err = cells_node_free(cells, allocation.index, allocation.size);
      if (free_err != ERROR_SUCCESS) { err = ERROR_INTERNAL; }
    }
  }
  stbds_arrfree(build.allocations);
  return err;
}

size_t bytecode_new_node0(struct bytecode_tree_builder_t* builder, struct cells_node_t payload) {
  struct bytecode_tree_t node = {
      .payload = payload, .left = {.has_value = false}, .right = {.has_value = false}};
  stbds_arrput(builder->nodes, node);
  return stbds_arrlen(builder->nodes) - 1;
}

size_t bytecode_new_node1(
    struct bytecode_tree_builder_t* builder, struct cells_node_t payload, size_t left) {
  struct bytecode_tree_t node = {
      .payload = payload,
      .left = {.has_value = true, .value = left},
      .right = {.has_value = false}};
  stbds_arrput(builder->nodes, node);
  return stbds_arrlen(builder->nodes) - 1;
}

size_t bytecode_new_node2(
    struct bytecode_tree_builder_t* builder,
    struct cells_node_t payload,
    size_t left,
    size_t right) {
  struct bytecode_tree_t node = {
      .payload = payload,
      .left = {.has_value = true, .value = left},
      .right = {.has_value = true, .value = right}};
  stbds_arrput(builder->nodes, node);
  return stbds_arrlen(builder->nodes) - 1;
}
