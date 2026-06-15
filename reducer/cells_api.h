#ifndef __REDUCER_CELLS_API_H__
#define __REDUCER_CELLS_API_H__

#include "domain_api.h"
#include <stddef.h>

struct cells_t;

/*
 * Semantic node identities. Values are C API identities, not bytecode tags.
 * X(PREFIX, NAME, VALUE, STRING)
 */
#define CELLS_NODE_TYPE_ITEMS(X, P)                                                                \
  X(P, INVALID, 0x00, "invalid")                                                                   \
  X(P, DELTA0, 0x01, "delta0")                                                                     \
  X(P, DELTA1, 0x02, "delta1")                                                                     \
  X(P, DELTA2, 0x03, "delta2")                                                                     \
  X(P, VALUEF0, 0x04, "valuef0")                                                                   \
  X(P, VALUEF1, 0x05, "valuef1")                                                                   \
  X(P, VALUEF2, 0x06, "valuef2")                                                                   \
  X(P, VALUEV0, 0x07, "valuev0")                                                                   \
  X(P, VALUEV1, 0x08, "valuev1")                                                                   \
  X(P, VALUEV2, 0x09, "valuev2")                                                                   \
  X(P, APPLY, 0x0A, "$")                                                                           \
  X(P, OP_FN0, 0x0B, "fn0")                                                                        \
  X(P, OP_FN1, 0x0C, "fn1")                                                                        \
  X(P, OP_FN2, 0x0D, "fn2")                                                                        \
  X(P, REF, 0xF0, "ref")

DECL_TYPED_ENUM(cells_node_type_t, u8, CELLS_NODE_TYPE, CELLS_NODE_TYPE_ITEMS)

#define CELLS_NODE_ARITY_NONE (-1)

struct cells_node_header_t {
  cells_node_type_t type;
  size_t encoded_size;
};

MUH_PUBLIC i8 cells_node_type_get_arity(cells_node_type_t type);
MUH_PUBLIC cells_node_type_t cells_node_type_with_arity(cells_node_type_t type, u8 arity);
MUH_PUBLIC bool cells_node_type_is_ref(cells_node_type_t type);
MUH_PUBLIC bool cells_node_type_is_encodable(cells_node_type_t type);

struct cells_node_t {
  struct cells_node_header_t header;

  union {
    i64 ref;
    i64 nativef;
    span_byte_t nativev;
  } as;
};

MUH_PUBLIC bool cells_node_set_arity(struct cells_node_t* node, u8 arity);

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

/** Get read-only span over full cells capacity, including free bytes. */
MUH_PUBLIC span_cbyte_t cells_get_span(const struct cells_t* cells);

/**
 * Get decoded header of the node at the specified index in the cells structure.
 * Calls to this function parse internal representation of the node, so be
 * careful doing it frequently.
 * @return cells_node_header_t on success, or an invalid header on failure.
 */
MUH_PUBLIC struct cells_node_header_t cells_get_node_header(struct cells_t* cells, size_t index);

/**
 * Get the value at the specified index in the cells structure.
 * @return cells_node_t on success, or an invalid node on failure.
 */
MUH_PUBLIC struct cells_node_t cells_get_node(
    struct cells_t* cells, size_t index, struct cells_node_header_t header);

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

/** Construct a payloadless node whose wire layout is TAG. */
MUH_PUBLIC struct cells_node_t cells_new_node(cells_node_type_t type);
MUH_PUBLIC struct cells_node_t cells_new_ref(i64 offset);
MUH_PUBLIC struct cells_node_t cells_new_delta0(void);
MUH_PUBLIC struct cells_node_t cells_new_delta1(void);
MUH_PUBLIC struct cells_node_t cells_new_delta2(void);
MUH_PUBLIC struct cells_node_t cells_new_value0f(i64 value);
MUH_PUBLIC struct cells_node_t cells_new_value1f(i64 value);
MUH_PUBLIC struct cells_node_t cells_new_value2f(i64 value);
MUH_PUBLIC struct cells_node_t cells_new_value0v(span_byte_t payload);
MUH_PUBLIC struct cells_node_t cells_new_value1v(span_byte_t payload);
MUH_PUBLIC struct cells_node_t cells_new_value2v(span_byte_t payload);

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
