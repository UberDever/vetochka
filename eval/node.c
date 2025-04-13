#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#if 0
#include "common.h"

#define NODE_SIZE (sizeof(uint) * 8)
#define TAG_SIZE  (uint)2
#define DATA_SIZE ((NODE_SIZE - TAG_SIZE) / 2)
/* #define DATA_MAX     ~(1 << DATA_SIZE) */
#define DATA_INVALID (~(uint)0 & CHILD_MASK)

#define TAG_MASK   (uint)(((uint)1 << TAG_SIZE) - 1)
#define CHILD_MASK (uint)(((uint)1 << DATA_SIZE) - 1)
#define DATA_MASK  (uint)(((uint)1 << (NODE_SIZE - TAG_SIZE)) - 1)

uint node_tag_tree() {
  return NODE_TREE;
}

uint node_tag_app() {
  return NODE_APP;
}

uint node_tag_data() {
  return NODE_DATA;
}

static inline Node node_new(uint tag, uint lhs, uint rhs) {
  Node node = 0;
  node &= ~TAG_MASK;
  node |= (uint)(tag & TAG_MASK);
  node &= ~(uint)(CHILD_MASK << TAG_SIZE);
  node |= (uint)((lhs & CHILD_MASK) << TAG_SIZE);
  node &= ~(uint)(CHILD_MASK << (TAG_SIZE + DATA_SIZE));
  node |= (uint)((rhs & CHILD_MASK) << (TAG_SIZE + DATA_SIZE));
  return node;
}

Node node_new_tree(sint lhs, sint rhs) {
  return node_new(NODE_TREE, lhs, rhs);
}

Node node_new_app(uint lhs, uint rhs) {
  return node_new(NODE_APP, lhs, rhs);
}

Node node_new_data(uint data) {
  Node node = 0;
  node &= ~TAG_MASK;
  node |= (uint)(NODE_DATA & TAG_MASK);
  node |= (uint)((data & DATA_MASK) << TAG_SIZE);
  return node;
}

uint node_new_invalid() {
  return DATA_INVALID;
}

uint node_tag(Node node) {
  return node & TAG_MASK;
}

sint node_lhs(Node node) {
  return (node >> TAG_SIZE) & CHILD_MASK;
}

sint node_rhs(Node node) {
  return (node >> (TAG_SIZE + DATA_SIZE)) & CHILD_MASK;
}

uint node_data(Node node) {
  return (node >> TAG_SIZE) & DATA_MASK;
}

#endif