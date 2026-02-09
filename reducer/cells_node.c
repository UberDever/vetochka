#include "cells_api.h"
#include "cells_impl.h"

struct cells_node_t cells_new_ref1(i8 offset) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_REF1;
  node.meta.size = sizeof(offset);
  node.as.ref = offset;
  return node;
}

struct cells_node_t cells_new_ref2(i16 offset) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_REF2;
  node.meta.size = sizeof(offset);
  node.as.ref = offset;
  return node;
}

struct cells_node_t cells_new_ref4(i32 offset) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_REF4;
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

struct cells_node_t cells_new_tree0() {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_TREE0;
  node.meta.size = 1;
  return node;
}

struct cells_node_t cells_new_tree1() {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_TREE1;
  node.meta.size = 1;
  return node;
}

struct cells_node_t cells_new_tree2() {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_TREE2;
  node.meta.size = 1;
  return node;
}

struct cells_node_t cells_new_native0f(i64 value) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_NATIVE0F;
  node.meta.size = 1 + sizeof(value);
  node.as.nativef = value;
  return node;
}

struct cells_node_t cells_new_native1f(i64 value) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_NATIVE1F;
  node.meta.size = 1 + sizeof(value);
  node.as.nativef = value;
  return node;
}

struct cells_node_t cells_new_native2f(i64 value) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_NATIVE2F;
  node.meta.size = 1 + sizeof(value);
  node.as.nativef = value;
  return node;
}

struct cells_node_t cells_new_native0v(span_byte_t payload) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_NATIVE0V;
  node.as.nativev = payload;
  node.meta.size = 1 + 1 + uleb128_size(node.as.nativev.len >> 5) + node.as.nativev.len;
  return node;
}

struct cells_node_t cells_new_native1v(span_byte_t payload) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_NATIVE1V;
  node.as.nativev = payload;
  node.meta.size = 1 + 1 + uleb128_size(node.as.nativev.len >> 5) + node.as.nativev.len;
  return node;
}

struct cells_node_t cells_new_native2v(span_byte_t payload) {
  struct cells_node_t node = {0};
  node.meta.type = CELLS_NODE_TYPE_NATIVE2V;
  node.as.nativev = payload;
  node.meta.size = 1 + 1 + uleb128_size(node.as.nativev.len >> 5) + node.as.nativev.len;
  return node;
}

bool cells_is_ref(struct cells_node_meta_t meta) {
  return meta.type == CELLS_NODE_TYPE_REF1 || meta.type == CELLS_NODE_TYPE_REF2
         || meta.type == CELLS_NODE_TYPE_REF4 || meta.type == CELLS_NODE_TYPE_REF8;
}

bool cells_is_native(struct cells_node_meta_t meta) {
  return meta.type == CELLS_NODE_TYPE_NATIVE0F || meta.type == CELLS_NODE_TYPE_NATIVE1F
         || meta.type == CELLS_NODE_TYPE_NATIVE2F || meta.type == CELLS_NODE_TYPE_NATIVE0V
         || meta.type == CELLS_NODE_TYPE_NATIVE1V || meta.type == CELLS_NODE_TYPE_NATIVE2V;
}

bool cells_is_opcode(struct cells_node_meta_t meta) {
  bool is_native = cells_is_native(meta);
  bool is_ref = cells_is_ref(meta);
  bool is_tree =
      (meta.type == CELLS_NODE_TYPE_TREE0 || meta.type == CELLS_NODE_TYPE_TREE1
       || meta.type == CELLS_NODE_TYPE_TREE2);
  return !is_native && !is_ref && !is_tree;
}
