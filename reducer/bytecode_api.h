#ifndef __REDUCER_BYTECODE_API_H__
#define __REDUCER_BYTECODE_API_H__

#include "typedefs.h"

struct cells_node_t;
struct cells_t;
struct bytecode_tree_builder_t;

error_t bytecode_tree_builder_create(struct bytecode_tree_builder_t **builder);
void bytecode_tree_builder_destroy(struct bytecode_tree_builder_t **builder);
void bytecode_tree_builder_reset(struct bytecode_tree_builder_t *builder);
size_t bytecode_new_node0(struct bytecode_tree_builder_t *builder,
                          struct cells_node_t payload);
size_t bytecode_new_node1(struct bytecode_tree_builder_t *builder,
                          struct cells_node_t payload, size_t left);
size_t bytecode_new_node2(struct bytecode_tree_builder_t *builder,
                          struct cells_node_t payload, size_t left,
                          size_t right);
error_t bytecode_tree_builder_build(struct bytecode_tree_builder_t *builder,
                                    struct cells_t *cells, size_t *index_out);

#endif // __REDUCER_BYTECODE_API_H__
