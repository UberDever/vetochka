#ifndef __REDUCER_BYTECODE_API_H__
#define __REDUCER_BYTECODE_API_H__

#include "domain_api.h"

struct cells_node_t;
struct cells_t;
struct bytecode_tree_builder_t;
struct reducer_t;
struct source_tree_t;

MUH_PUBLIC error_t bytecode_tree_builder_create(struct bytecode_tree_builder_t** builder);
MUH_PUBLIC void bytecode_tree_builder_destroy(struct bytecode_tree_builder_t** builder);
MUH_PUBLIC void bytecode_tree_builder_reset(struct bytecode_tree_builder_t* builder);
MUH_PUBLIC size_t
bytecode_new_node0(struct bytecode_tree_builder_t* builder, struct cells_node_t payload);
MUH_PUBLIC size_t bytecode_new_node1(
    struct bytecode_tree_builder_t* builder, struct cells_node_t payload, size_t left);
MUH_PUBLIC size_t bytecode_new_node2(
    struct bytecode_tree_builder_t* builder,
    struct cells_node_t payload,
    size_t left,
    size_t right);
MUH_PUBLIC error_t bytecode_tree_builder_build(
    struct bytecode_tree_builder_t* builder, struct cells_t* cells, size_t* index_out);
MUH_PUBLIC error_t bytecode_source_encode(
    span_cbyte_t text,
    const struct source_tree_t* source,
    struct cells_t* cells,
    size_t* index_out);

#endif // __REDUCER_BYTECODE_API_H__
