#include "bytecode_api.h"
#include "cells_api.h"
#include "typedefs.h"
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

error_t bytecode_tree_builder_create(struct bytecode_tree_builder_t** builder) {
  *builder = calloc(1, sizeof(struct bytecode_tree_builder_t));
  if (!*builder) { return ERROR_GENERIC; }
  return ERROR_SUCCESS;
}

void bytecode_tree_builder_destroy(struct bytecode_tree_builder_t** builder) {
  stbds_arrfree((*builder)->nodes);
  free(*builder);
}

void bytecode_tree_builder_reset(struct bytecode_tree_builder_t* builder) {
  stbds_arrsetlen(builder->nodes, 0);
}

error_t do_build(
    struct bytecode_tree_builder_t* b,
    struct cells_t* cells,
    struct opt_size_t root_i,
    size_t* index_out) {
  error_t err = ERROR_SUCCESS;
  if (!root_i.has_value) { return ERROR_SUCCESS; }
  struct bytecode_tree_t root = b->nodes[root_i.value];

  size_t left_i = SIZE_MAX;
  err = do_build(b, cells, root.left, &left_i);
  if (err != ERROR_SUCCESS) { return err; }
  size_t right_i = SIZE_MAX;
  err = do_build(b, cells, root.right, &right_i);
  if (err != ERROR_SUCCESS) { return err; }

  if (left_i == SIZE_MAX && right_i == SIZE_MAX) {
    err = cells_alloc_chunk(cells, root.payload.meta.size, index_out);
    if (err != ERROR_SUCCESS) { return err; }
    return cells_write_node(cells, *index_out, root.payload);
  }
  assert(left_i != SIZE_MAX);

  struct opt_size_t left_index = {0};
  struct opt_size_t right_index = {0};
  left_index = (struct opt_size_t){.has_value = true, .value = left_i};
  if (right_i != SIZE_MAX) {
    right_index = (struct opt_size_t){.has_value = true, .value = right_i};
  }

  err = cells_alloc_chunk_with_refs(
      cells, root.payload.meta.size, left_index, right_index, index_out);
  if (err != ERROR_SUCCESS) { return err; }
  err = cells_write_node(cells, *index_out, root.payload);
  if (err != ERROR_SUCCESS) { return err; }

  i64 lhs_ref_i = (i64)left_i - (i64)(*index_out + root.payload.meta.size);
  cells_node_t lhs_ref;
  if (bytecode_fits_in_ref2(lhs_ref_i)) {
    lhs_ref = bytecode_new_ref2(lhs_ref_i);
  } else {
    assert(bytecode_fits_in_ref8(lhs_ref_i));
    lhs_ref = bytecode_new_ref8(lhs_ref_i);
  }
  err = cells_write_node(cells, *index_out + root.payload.meta.size, lhs_ref);
  if (err != ERROR_SUCCESS) { return err; }

  if (right_i == SIZE_MAX) { return err; }

  i64 rhs_ref_i = (i64)right_i - (i64)(*index_out + root.payload.meta.size + lhs_ref.meta.size);
  cells_node_t rhs_ref;
  if (bytecode_fits_in_ref2(rhs_ref_i)) {
    rhs_ref = bytecode_new_ref2(rhs_ref_i);
  } else {
    assert(bytecode_fits_in_ref8(rhs_ref_i));
    rhs_ref = bytecode_new_ref8(rhs_ref_i);
  }
  err = cells_write_node(cells, *index_out + root.payload.meta.size + lhs_ref.meta.size, rhs_ref);
  if (err != ERROR_SUCCESS) { return err; }

  return err;
}

error_t bytecode_tree_builder_build(
    struct bytecode_tree_builder_t* builder, struct cells_t* cells, size_t* index_out) {
  size_t len = stbds_arrlenu(builder->nodes);
  if (len == 0) { return ERROR_SUCCESS; }
  return do_build(
      builder, cells, (struct opt_size_t){.has_value = true, .value = len - 1}, index_out);
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
