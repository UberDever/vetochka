#ifndef __REDUCER_CELLS_API_H__
#define __REDUCER_CELLS_API_H__

#include "domain_api.h"
#include <stddef.h>

struct cells_t;

struct CELLS_NODE_TYPE {
  u8 value;
};

#define CELLS_NODE_TYPE_INVALID 0U
#define CELLS_NODE_TYPE_DELTA0  1U
#define CELLS_NODE_TYPE_DELTA1  2U
#define CELLS_NODE_TYPE_DELTA2  3U
#define CELLS_NODE_TYPE_VALUEF0 4U
#define CELLS_NODE_TYPE_VALUEF1 5U
#define CELLS_NODE_TYPE_VALUEF2 6U
#define CELLS_NODE_TYPE_VALUEV0 7U
#define CELLS_NODE_TYPE_VALUEV1 8U
#define CELLS_NODE_TYPE_VALUEV2 9U

#define CELLS_NODE_TYPE_REF2 0xF0U
#define CELLS_NODE_TYPE_REF8 0xF1U

struct cells_node_meta_t {
  struct CELLS_NODE_TYPE type;
  size_t size;
};

MUH_PUBLIC bool cells_is_ref(struct cells_node_meta_t meta);
MUH_PUBLIC bool cells_is_value(struct cells_node_meta_t meta);

struct cells_node_t {
  struct cells_node_meta_t meta;

  union {
    i64 ref;
    i64 nativef;
    span_byte_t nativev;
  } as;
};

struct opt_cells_node_t {
  bool has_value;
  struct cells_node_t value;
};

/**
 * Alloc and initialize a cells_t structure with the given capacity.
 * @return ERROR_SUCCESS on success, or error code on failure.
 */
MUH_PUBLIC error_t cells_create(struct cells_t** cells, size_t capacity);

/**
 * Deinit and free the cells structure.
 */
MUH_PUBLIC void cells_destroy(struct cells_t** cells);

/**
 * Get meta of the node at the specified index in the cells structure.
 * Calls to this function parse internal representation of the node, so be
 * careful doing it frequently.
 * @return cells_node_meta_t on success, or an invalid node on failure.
 */
MUH_PUBLIC struct cells_node_meta_t cells_get_node_meta(struct cells_t* cells, size_t index);

/**
 * Get the value at the specified index in the cells structure.
 * @return cells_node_t on success, or an invalid node on failure.
 */
MUH_PUBLIC struct cells_node_t cells_get_node(
    struct cells_t* cells, size_t index, struct cells_node_meta_t meta);

/**
 * Allocate a chunk of memory with the specified size and get its index.
 * Marks the cells as occupied.
 * @return ERROR_SUCCESS on success, error code on failure.
 */
MUH_PUBLIC error_t cells_alloc_chunk(struct cells_t* cells, size_t chunk_size, size_t* index_out);

/**
 * Tries to allocate a chunk of memory for specified referenced node at index.
 * Tries different refs of different sizes, until the resulted chunk will be
 * able to store references of sufficient sizes.
 * @param chunk_size must include size of the node, without size for the
 * references, so algorighm will calclulate those for you.
 * @param referenced_lhs and referenced_rhs are the indices of the nodes you
 * need to reference; it is expected that either lhs or (lhs, rhs) will be set.
 * @param index_out will be the start of the allocated chunk, shifts for the
 * references must be calclulated by the caller based on the returned index and
 * node size.
 * @return ERROR_SUCCESS on success, error code on failure.
 */
MUH_PUBLIC error_t cells_alloc_chunk_with_refs(
    struct cells_t* cells,
    size_t chunk_size,
    struct opt_size_t referenced_lhs,
    struct opt_size_t referenced_rhs,
    size_t* index_out);

/**
 * Writes node inplace at provided index.
 * Expects that memory was allocated for specified type of node at specified
 * index. Note that if node at this index is not of type that is expected, this
 * would result in memory override or even out of bounds UB.
 * @return ERROR_SUCCESS on success, error code on failure.
 */
MUH_PUBLIC error_t cells_write_node(struct cells_t* cells, size_t index, struct cells_node_t node);

/**
 * Mark the chunk of memory at the specified index of node size as free.
 * @return ERROR_SUCCESS on success, or error code on failure.
 */
MUH_PUBLIC error_t cells_node_free(struct cells_t* cells, size_t index, size_t node_size);

// ---------------------------------------------------------------------------
// Node constructors
// ---------------------------------------------------------------------------

#define ON_NODE_ARITY0                                                                             \
  case CELLS_NODE_TYPE_VALUEF0:                                                                    \
  case CELLS_NODE_TYPE_VALUEV0:                                                                    \
  case CELLS_NODE_TYPE_DELTA0:

#define ON_NODE_ARITY1                                                                             \
  case CELLS_NODE_TYPE_VALUEF1:                                                                    \
  case CELLS_NODE_TYPE_VALUEV1:                                                                    \
  case CELLS_NODE_TYPE_DELTA1:

#define ON_NODE_ARITY2                                                                             \
  case CELLS_NODE_TYPE_VALUEF2:                                                                    \
  case CELLS_NODE_TYPE_VALUEV2:                                                                    \
  case CELLS_NODE_TYPE_DELTA2:

MUH_PUBLIC struct cells_node_t cells_new_ref2(i16 offset);
MUH_PUBLIC struct cells_node_t cells_new_ref8(i64 offset);
MUH_PUBLIC struct cells_node_t cells_new_delta0(void);
MUH_PUBLIC struct cells_node_t cells_new_delta1(void);
MUH_PUBLIC struct cells_node_t cells_new_delta2(void);
MUH_PUBLIC struct cells_node_t cells_new_value0f(i64 value);
MUH_PUBLIC struct cells_node_t cells_new_value1f(i64 value);
MUH_PUBLIC struct cells_node_t cells_new_value2f(i64 value);
MUH_PUBLIC struct cells_node_t cells_new_value0v(span_byte_t payload);
MUH_PUBLIC struct cells_node_t cells_new_value1v(span_byte_t payload);
MUH_PUBLIC struct cells_node_t cells_new_value2v(span_byte_t payload);

MUH_PUBLIC bool cells_fits_in_ref2(i64 value);
MUH_PUBLIC bool cells_fits_in_ref8(i64 value);

// NOTE: this allows ref to ref, and doesn't handle cycles.
// cycle currently considered as malformed bytecode, so hanging is abnormal
// behavior. changes index to point at the dereferenced node
MUH_PUBLIC error_t
cells_dereference_node(struct cells_t* cells, size_t* index, struct cells_node_t* out_node);
MUH_PUBLIC error_t
cells_get_left_node(struct cells_t* cells, size_t* parent_index, struct cells_node_t* out_node);
MUH_PUBLIC error_t
cells_get_right_node(struct cells_t* cells, size_t* parent_index, struct cells_node_t* out_node);

typedef void (*cells_print_fn)(void* ctx, const char* fmt, ...);

/**
 * Print a hex-view like visualization of the cells structure using the provided
 * print function. Left column: Hex dump of bytes, colored by node type. Right
 * column: Human-readable representation of nodes.
 */
MUH_PUBLIC void cells_print_debug_view(struct cells_t* cells, cells_print_fn print, void* ctx);

#endif // __REDUCER_CELLS_API_H__
