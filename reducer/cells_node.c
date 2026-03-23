#include "cells_api.h"
#include "cells_impl.h"
#include "typedefs.h"
#include <stdio.h>

typedef struct cells_node_t cells_node_t;
typedef struct cells_node_meta_t cells_node_meta_t;

cells_node_t cells_new_ref2(i16 offset) {
  cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_REF2;
  node.meta.size = sizeof(offset);
  node.as.ref = offset;
  return node;
}

cells_node_t cells_new_ref8(i64 offset) {
  cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_REF8;
  node.meta.size = sizeof(offset);
  node.as.ref = offset;
  return node;
}

cells_node_t cells_new_delta0(void) {
  cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_DELTA0;
  node.meta.size = 1;
  return node;
}

cells_node_t cells_new_delta1(void) {
  cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_DELTA1;
  node.meta.size = 1;
  return node;
}

cells_node_t cells_new_delta2(void) {
  cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_DELTA2;
  node.meta.size = 1;
  return node;
}

cells_node_t cells_new_value0f(i64 value) {
  cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_VALUEF0;
  node.meta.size = 1 + sizeof(value);
  node.as.nativef = value;
  return node;
}

cells_node_t cells_new_value1f(i64 value) {
  cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_VALUEF1;
  node.meta.size = 1 + sizeof(value);
  node.as.nativef = value;
  return node;
}

cells_node_t cells_new_value2f(i64 value) {
  cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_VALUEF2;
  node.meta.size = 1 + sizeof(value);
  node.as.nativef = value;
  return node;
}

cells_node_t cells_new_value0v(span_byte_t payload) {
  cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_VALUEV0;
  node.as.nativev = payload;
  node.meta.size = 1 + uleb128_size(node.as.nativev.len) + node.as.nativev.len;
  return node;
}

cells_node_t cells_new_value1v(span_byte_t payload) {
  cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_VALUEV1;
  node.as.nativev = payload;
  node.meta.size = 1 + uleb128_size(node.as.nativev.len) + node.as.nativev.len;
  return node;
}

cells_node_t cells_new_value2v(span_byte_t payload) {
  cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_VALUEV2;
  node.as.nativev = payload;
  node.meta.size = 1 + uleb128_size(node.as.nativev.len) + node.as.nativev.len;
  return node;
}

bool cells_is_ref(struct cells_node_meta_t meta) {
  return meta.type == CELLS_NODE_TYPE_REF2 || meta.type == CELLS_NODE_TYPE_REF8;
}

bool cells_is_value(struct cells_node_meta_t meta) {
  return meta.type == CELLS_NODE_TYPE_VALUEF0 || meta.type == CELLS_NODE_TYPE_VALUEF1
         || meta.type == CELLS_NODE_TYPE_VALUEF2 || meta.type == CELLS_NODE_TYPE_VALUEV0
         || meta.type == CELLS_NODE_TYPE_VALUEV1 || meta.type == CELLS_NODE_TYPE_VALUEV2;
}

bool cells_fits_in_ref2(i64 value) {
  return value <= 0x1fff && value >= -0x1fff;
}

bool cells_fits_in_ref8(i64 value) {
  return value <= 0x1fffffffffffffff && value >= -0x1fffffffffffffff;
}

error_t cells_dereference_node(cells_t* cells, size_t* index, cells_node_t* out_node) {
  while (1) {
    cells_node_meta_t meta = cells_get_node_meta(cells, *index);
    if (meta.type == CELLS_NODE_TYPE_INVALID) { return ERROR_GENERIC; }
    *out_node = cells_get_node(cells, *index, meta);
    if (out_node->meta.type == CELLS_NODE_TYPE_INVALID) { return ERROR_GENERIC; }
    if (meta.type == CELLS_NODE_TYPE_REF2 || meta.type == CELLS_NODE_TYPE_REF8) {
      *index += out_node->as.ref;
      continue;
    }
    return ERROR_SUCCESS;
  }
}

error_t cells_get_left_node(cells_t* cells, size_t* parent_index, cells_node_t* out_node) {
  cells_node_meta_t parent_meta = cells_get_node_meta(cells, *parent_index);
  size_t index = *parent_index + parent_meta.size;
  error_t err = cells_dereference_node(cells, &index, out_node);
  if (err != ERROR_SUCCESS) { return err; }
  *parent_index = index;
  return ERROR_SUCCESS;
}

error_t cells_get_right_node(cells_t* cells, size_t* parent_index, cells_node_t* out_node) {
  cells_node_meta_t parent_meta = cells_get_node_meta(cells, *parent_index);
  cells_node_meta_t meta = cells_get_node_meta(cells, *parent_index + parent_meta.size);
  if (meta.type == CELLS_NODE_TYPE_INVALID) { return ERROR_GENERIC; }
  if (!(meta.type == CELLS_NODE_TYPE_REF2 || meta.type == CELLS_NODE_TYPE_REF8)) {
    return ERROR_GENERIC;
  }

  size_t index = *parent_index + parent_meta.size + meta.size;

  error_t err = cells_dereference_node(cells, &index, out_node);
  if (err != ERROR_SUCCESS) { return err; }
  *parent_index = index;
  return ERROR_SUCCESS;
}
