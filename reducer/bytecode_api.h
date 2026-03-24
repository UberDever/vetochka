#ifndef __REDUCER_BYTECODE_API_H__
#define __REDUCER_BYTECODE_API_H__

#include "typedefs.h"

struct cells_node_t;
struct cells_t;
struct bytecode_tree_builder_t;
struct reducer_t;

error_t bytecode_tree_builder_create(struct bytecode_tree_builder_t** builder);
void bytecode_tree_builder_destroy(struct bytecode_tree_builder_t** builder);
void bytecode_tree_builder_reset(struct bytecode_tree_builder_t* builder);
size_t bytecode_new_node0(struct bytecode_tree_builder_t* builder,
                          struct cells_node_t payload);
size_t bytecode_new_node1(struct bytecode_tree_builder_t* builder,
                          struct cells_node_t payload, size_t left);
size_t bytecode_new_node2(struct bytecode_tree_builder_t* builder,
                          struct cells_node_t payload, size_t left,
                          size_t right);
error_t bytecode_tree_builder_build(struct bytecode_tree_builder_t* builder,
                                    struct cells_t* cells, size_t* index_out);

struct bytecode_reading_result_t;

/*
 * Compile provided text to bytecode cells, updating reducer stack to encode tree applications.
 *
 * @param src Input text. Can be cleaned up after the call the function, as payload is copied into cells.
 * @param cells Destination cells store.
 * @param applications_stack Stack that stores applications in forms of indices and tokens REDUCER_APPLY_TOKEN. Expected to be a stbds array.
 * @param result Store the results of compilation.
 */
error_t bytecode_text_read(span_cbyte_t src, struct cells_t* cells, size_t* applications_stack, struct bytecode_reading_result_t* result);

struct bytecode_writing_result_t;

/*
 * Write bytecode tree to text.
 *
 * @param cells Source cells store.
 * @param applications_stack Stack that stores applications in forms of indices and tokens REDUCER_APPLY_TOKEN. Expected to be a stbds array.
 * @param dst Buffer to write the result to. If the size of buffer is smol, return ERROR_NOMEM
 */
error_t bytecode_text_write(struct cells_t* cells, size_t* applications_stack, span_byte_t dst);

#endif // __REDUCER_BYTECODE_API_H__
