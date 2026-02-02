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
} cells_node_meta_t;

struct cells_node_t {
    struct cells_node_meta_t meta;
    union {
        i64 ref;
        i64 nativef;
        span_byte_t nativev;
    } as;
};

/**
 * Initialize a cells_t structure with the given capacity.
 * @return 0 on success, or -1 on failure.
 */
int cells_init(struct cells_t** cells, size_t capacity);

/**
 * Free the cells structure.
 */
void cells_free(struct cells_t** cells);

/**
 * Get meta of the node at the specified index in the cells structure.
 * Calls to this function parse internal representation of the node, so be careful doing it frequently.
 * @return cells_node_meta_t on success, or an invalid node on failure.
 */
struct cells_node_meta_t cells_get_node_meta(struct cells_t* cells, size_t index);

/**
 * Get the value at the specified index in the cells structure.
 * @return cells_node_t on success, or an invalid node on failure.
 */
struct cells_node_t cells_get_node(struct cells_t* cells, size_t index, struct cells_node_meta_t meta);

/**
 * Allocate node with current metadata and get its index.
 * Marks bytes of the node as occupied.
 * @return 0 on success, -1 on failure.
 */
int cells_alloc_node(struct cells_t* cells, size_t node_size, size_t* index_out);

/**
 * Writes node inplace at provided index.
 * Expects that memory was allocated for specified type of node at specified index.
 * Note that if node at this index is not of type that is expected, this would result in memory override
 * or even out of bounds UB.
 * @return 0 on success, -1 on failure.
 */
int cells_write_node(struct cells_t* cells, size_t index, struct cells_node_t node);

/**
 * Mark the chunk of memory at the specified index of node size as free.
 * @return 0 on success, or -1 on failure.
 */
int cells_node_free(struct cells_t* cells, size_t index, size_t node_size);

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

#endif // __REDUCER_CELLS_API_H__
