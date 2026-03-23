#define __REDUCER_BYTECODE_API_H__

#include "typedefs.h"

struct cells_t;

// NOTE: deliberately not using CELLS_NODE_TYPE here because
// there will be less places to make a mistake
struct cells_node_t bytecode_new_ref2(i16 offset);
struct cells_node_t bytecode_new_ref8(i64 offset);
struct cells_node_t bytecode_new_delta0();
struct cells_node_t bytecode_new_delta1();
struct cells_node_t bytecode_new_delta2();
struct cells_node_t bytecode_new_value0f(i64 value);
struct cells_node_t bytecode_new_value1f(i64 value);
struct cells_node_t bytecode_new_value2f(i64 value);
struct cells_node_t bytecode_new_value0v(span_byte_t payload);
struct cells_node_t bytecode_new_value1v(span_byte_t payload);
struct cells_node_t bytecode_new_value2v(span_byte_t payload);

bool bytecode_fits_in_ref2(i64 value);
bool bytecode_fits_in_ref8(i64 value);

// NOTE: this allows ref to ref, and doesn't handle cycles.
// cycle currently considered as malformed bytecode, so hanging is abnormal
// behavior. changes index to point at the dereferenced node
error_t bytecode_dereference_node(struct cells_t *cells, size_t *index,
                                  struct cells_node_t *out_node);
error_t bytecode_get_left_node(struct cells_t *cells, size_t *parent_index,
                               struct cells_node_t *out_node);
error_t bytecode_get_right_node(struct cells_t *cells, size_t *parent_index,
                                struct cells_node_t *out_node);

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
