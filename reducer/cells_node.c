#include "cells_api.h"
#include "cells_impl.h"

struct cells_node_t cells_new_ref2(i16 offset) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_REF2;
  node.meta.size = sizeof(offset);
  node.as.ref = offset;
  return node;
}

struct cells_node_t cells_new_ref8(i64 offset) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_REF8;
  node.meta.size = sizeof(offset);
  node.as.ref = offset;
  return node;
}

struct cells_node_t cells_new_delta0() {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_DELTA0;
  node.meta.size = 1;
  return node;
}

struct cells_node_t cells_new_delta1() {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_DELTA1;
  node.meta.size = 1;
  return node;
}

struct cells_node_t cells_new_delta2() {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_DELTA2;
  node.meta.size = 1;
  return node;
}

struct cells_node_t cells_new_value0f(i64 value) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_VALUEF0;
  node.meta.size = 1 + sizeof(value);
  node.as.nativef = value;
  return node;
}

struct cells_node_t cells_new_value1f(i64 value) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_VALUEF1;
  node.meta.size = 1 + sizeof(value);
  node.as.nativef = value;
  return node;
}

struct cells_node_t cells_new_value2f(i64 value) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_VALUEF2;
  node.meta.size = 1 + sizeof(value);
  node.as.nativef = value;
  return node;
}

struct cells_node_t cells_new_value0v(span_byte_t payload) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_VALUEV0;
  node.as.nativev = payload;
  node.meta.size = 1 + uleb128_size(node.as.nativev.len) + node.as.nativev.len;
  return node;
}

struct cells_node_t cells_new_value1v(span_byte_t payload) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_VALUEV1;
  node.as.nativev = payload;
  node.meta.size = 1 + uleb128_size(node.as.nativev.len) + node.as.nativev.len;
  return node;
}

struct cells_node_t cells_new_value2v(span_byte_t payload) {
  struct cells_node_t node = {0};
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

bool cells_is_opcode(struct cells_node_meta_t meta) {
  bool is_value = cells_is_value(meta);
  bool is_ref = cells_is_ref(meta);
  bool is_tree =
      (meta.type == CELLS_NODE_TYPE_DELTA0 || meta.type == CELLS_NODE_TYPE_DELTA1
       || meta.type == CELLS_NODE_TYPE_DELTA2);
  return !is_value && !is_ref && !is_tree;
}
