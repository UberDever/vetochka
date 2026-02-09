#ifndef __REDUCER_CELLS_API_H__
#define __REDUCER_CELLS_API_H__

#include "typedefs.h"
#include <stddef.h>

struct cells_t;

enum CELLS_NODE_TYPE: u8 {
    CELLS_NODE_TYPE_INVALID = 0,
    CELLS_NODE_TYPE_TREE0,
    CELLS_NODE_TYPE_TREE1,
    CELLS_NODE_TYPE_TREE2,
    CELLS_NODE_TYPE_REF1,
    CELLS_NODE_TYPE_REF2,
    CELLS_NODE_TYPE_REF4,
    CELLS_NODE_TYPE_REF8,
    CELLS_NODE_TYPE_NATIVE0F,
    CELLS_NODE_TYPE_NATIVE1F,
    CELLS_NODE_TYPE_NATIVE2F,
    CELLS_NODE_TYPE_NATIVE0V,
    CELLS_NODE_TYPE_NATIVE1V,
    CELLS_NODE_TYPE_NATIVE2V,
    CELLS_NODE_TYPE_SEQ,
    CELLS_NODE_TYPE_SET,
    CELLS_NODE_TYPE_LAMBDA,
};

struct cells_node_meta_t {
    enum CELLS_NODE_TYPE type;
    size_t size;
};

struct cells_node_t {
    struct cells_node_meta_t meta;
    union {
        i64 ref;
        i64 nativef;
        span_byte_t nativev;
    } as;
};

/**
 * Alloc and initialize a cells_t structure with the given capacity.
 * @return ERROR_SUCCESS on success, or error code on failure.
 */
error_t cells_create(struct cells_t** cells, size_t capacity);

/**
 * Deinit and free the cells structure.
 */
void cells_destroy(struct cells_t** cells);

/**
 * Get meta of the node at the specified index in the cells structure.
 * Calls to this function parse internal representation of the node, so be
 * careful doing it frequently.
 * @return cells_node_meta_t on success, or an invalid node on failure.
 */
struct cells_node_meta_t cells_get_node_meta(struct cells_t* cells,
                                             size_t index);

/**
 * Get the value at the specified index in the cells structure.
 * @return cells_node_t on success, or an invalid node on failure.
 */
struct cells_node_t cells_get_node(struct cells_t* cells, size_t index,
                                   struct cells_node_meta_t meta);

/**
 * Allocate a chunk of memory with the specified size and get its index.
 * Marks the cells as occupied.
 * @return ERROR_SUCCESS on success, error code on failure.
 */
error_t cells_alloc_chunk(struct cells_t* cells, size_t chunk_size,
                          size_t* index_out);

/*
 * Tries to allocate a chunk of memory for specified referenced node at index.
 * Tries different refs of different sizes, until one of them will be able to
 * reference resulting chunk.
 */
// error_t cells_alloc_referenced_chunk(struct cells_t* cells, size_t total_size,
//                                      size_t referenced_index,
//                                      size_t* index_out, struct cells_node_t* ref_out);

/**
 * Writes node inplace at provided index.
 * Expects that memory was allocated for specified type of node at specified
 * index. Note that if node at this index is not of type that is expected, this
 * would result in memory override or even out of bounds UB.
 * @return ERROR_SUCCESS on success, error code on failure.
 */
error_t cells_write_node(struct cells_t* cells, size_t index,
                         struct cells_node_t node);

/**
 * Mark the chunk of memory at the specified index of node size as free.
 * @return ERROR_SUCCESS on success, or error code on failure.
 */
error_t cells_node_free(struct cells_t* cells, size_t index, size_t node_size);

// NOTE: deliberately not using CELLS_NODE_TYPE here because
// there will be less places to make a mistake
struct cells_node_t cells_new_ref1(i8 offset);
struct cells_node_t cells_new_ref2(i16 offset);
struct cells_node_t cells_new_ref4(i32 offset);
struct cells_node_t cells_new_ref8(i64 offset);
struct cells_node_t cells_new_tree0();
struct cells_node_t cells_new_tree1();
struct cells_node_t cells_new_tree2();
struct cells_node_t cells_new_native0f(i64 value);
struct cells_node_t cells_new_native1f(i64 value);
struct cells_node_t cells_new_native2f(i64 value);
struct cells_node_t cells_new_native0v(span_byte_t payload);
struct cells_node_t cells_new_native1v(span_byte_t payload);
struct cells_node_t cells_new_native2v(span_byte_t payload);

bool cells_is_ref(struct cells_node_meta_t meta);
bool cells_is_native(struct cells_node_meta_t meta);
bool cells_is_opcode(struct cells_node_meta_t meta);

#endif // __REDUCER_CELLS_API_H__
