#include "cells_api.h"
#include "cells_impl.h"
#include "domain_api.h"
#include <stdio.h>

typedef struct cells_node_t cells_node_t;
typedef struct cells_node_header_t cells_node_header_t;

i8 cells_node_type_get_arity(cells_node_type_t type) {
#define ARITY_IF(WIRE, TYPE, MASK, CODE, LAYOUT, ARITY, NEXT, SIZE)                                \
  if (type.value == CELLS_NODE_TYPE_##TYPE) { return ARITY; }

  CELLS_NODE_INFO_ITEMS(ARITY_IF)

#undef ARITY_IF

  return CELLS_NODE_ARITY_NONE;
}

static cells_node_type_t cells_node_type_get_next(cells_node_type_t type) {
#define NEXT_IF(WIRE, TYPE, MASK, CODE, LAYOUT, ARITY, NEXT, SIZE)                                 \
  if (type.value == CELLS_NODE_TYPE_##TYPE) {                                                      \
    return (cells_node_type_t){.value = CELLS_NODE_TYPE_##NEXT};                                   \
  }

  CELLS_NODE_INFO_ITEMS(NEXT_IF)

#undef NEXT_IF

  return (cells_node_type_t){.value = CELLS_NODE_TYPE_INVALID};
}

enum cells_node_layout_t cells_node_type_get_layout(cells_node_type_t type) {
#define LAYOUT_IF(WIRE, TYPE, MASK, CODE, LAYOUT, ARITY, NEXT, SIZE)                               \
  if (type.value == CELLS_NODE_TYPE_##TYPE) { return CELLS_NODE_LAYOUT_##LAYOUT; }

  CELLS_NODE_INFO_ITEMS(LAYOUT_IF)

#undef LAYOUT_IF

  return CELLS_NODE_LAYOUT_INVALID;
}

size_t cells_node_type_get_fixed_encoded_size(cells_node_type_t type) {
#define SIZE_IF(WIRE, TYPE, MASK, CODE, LAYOUT, ARITY, NEXT, SIZE)                                 \
  if (type.value == CELLS_NODE_TYPE_##TYPE) { return SIZE; }

  CELLS_NODE_INFO_ITEMS(SIZE_IF)

#undef SIZE_IF

  return 0;
}

bool cells_node_type_is_ref(cells_node_type_t type) {
  return type.value == CELLS_NODE_TYPE_REF;
}

bool cells_node_type_is_encodable(cells_node_type_t type) {
#define ENCODABLE_IF(WIRE, TYPE, MASK, CODE, LAYOUT, ARITY, NEXT, SIZE)                            \
  if (type.value == CELLS_NODE_TYPE_##TYPE) { return true; }

  CELLS_NODE_INFO_ITEMS(ENCODABLE_IF)

#undef ENCODABLE_IF

  return false;
}

static bool cells_node_type_reaches(cells_node_type_t from, cells_node_type_t to) {
  for (size_t i = 0; i < UINT8_MAX; i++) {
    if (from.value == to.value) { return true; }
    from = cells_node_type_get_next(from);
    if (from.value == CELLS_NODE_TYPE_INVALID) { return false; }
  }
  return false;
}

cells_node_type_t cells_node_type_with_arity(cells_node_type_t type, u8 arity) {
  if (cells_node_type_get_arity(type) == CELLS_NODE_ARITY_NONE) {
    return (cells_node_type_t){.value = CELLS_NODE_TYPE_INVALID};
  }

#define FIND_ARITY(WIRE, TYPE, MASK, CODE, LAYOUT, ARITY, NEXT, SIZE)                              \
  if ((i8)arity == ARITY) {                                                                        \
    cells_node_type_t candidate = {.value = CELLS_NODE_TYPE_##TYPE};                               \
    if (cells_node_type_reaches(type, candidate) || cells_node_type_reaches(candidate, type)) {    \
      return candidate;                                                                            \
    }                                                                                              \
  }

  CELLS_NODE_INFO_ITEMS(FIND_ARITY)

#undef FIND_ARITY

  return (cells_node_type_t){.value = CELLS_NODE_TYPE_INVALID};
}

bool cells_node_set_arity(struct cells_node_t* node, u8 arity) {
  if (!node) { return false; }
  cells_node_type_t type = cells_node_type_with_arity(node->header.type, arity);
  if (type.value == CELLS_NODE_TYPE_INVALID) { return false; }
  node->header.type = type;
  size_t fixed_encoded_size = cells_node_type_get_fixed_encoded_size(type);
  if (fixed_encoded_size != 0) { node->header.encoded_size = fixed_encoded_size; }
  return true;
}

static cells_node_t cells_new_fixed(cells_node_type_t type) {
  cells_node_t node = {0};
  size_t fixed_encoded_size = cells_node_type_get_fixed_encoded_size(type);
  assert(fixed_encoded_size != 0);
  node.header.type = type;
  node.header.encoded_size = fixed_encoded_size;
  return node;
}

cells_node_t cells_new_node(cells_node_type_t type) {
  if (cells_node_type_get_layout(type) != CELLS_NODE_LAYOUT_TAG) { return (cells_node_t){0}; }
  return cells_new_fixed(type);
}

bool cells_ref_fits_ref14(i64 value) {
  return value <= 0x1fff && value >= -0x2000;
}

bool cells_ref_fits_ref62(i64 value) {
  return value <= INT64_C(0x1fffffffffffffff) && value >= -INT64_C(0x2000000000000000);
}

cells_node_t cells_new_ref(i64 offset) {
  cells_node_t node = {0};
  node.header.type.value = CELLS_NODE_TYPE_REF;
  if (cells_ref_fits_ref14(offset)) {
    node.header.encoded_size = sizeof(i16);
  } else if (cells_ref_fits_ref62(offset)) {
    node.header.encoded_size = sizeof(i64);
  } else {
    return (cells_node_t){0};
  }
  node.as.ref = offset;
  return node;
}

cells_node_t cells_new_delta0(void) {
  return cells_new_node((cells_node_type_t){.value = CELLS_NODE_TYPE_DELTA0});
}

cells_node_t cells_new_delta1(void) {
  return cells_new_node((cells_node_type_t){.value = CELLS_NODE_TYPE_DELTA1});
}

cells_node_t cells_new_delta2(void) {
  return cells_new_node((cells_node_type_t){.value = CELLS_NODE_TYPE_DELTA2});
}

cells_node_t cells_new_value0f(i64 value) {
  cells_node_t node = cells_new_fixed((cells_node_type_t){.value = CELLS_NODE_TYPE_VALUEF0});
  node.as.nativef = value;
  return node;
}

cells_node_t cells_new_value1f(i64 value) {
  cells_node_t node = cells_new_fixed((cells_node_type_t){.value = CELLS_NODE_TYPE_VALUEF1});
  node.as.nativef = value;
  return node;
}

cells_node_t cells_new_value2f(i64 value) {
  cells_node_t node = cells_new_fixed((cells_node_type_t){.value = CELLS_NODE_TYPE_VALUEF2});
  node.as.nativef = value;
  return node;
}

static cells_node_t cells_new_variable(cells_node_type_t type, span_byte_t payload) {
  cells_node_t node = {0};
  node.header.type = type;
  node.as.nativev = payload;
  node.header.encoded_size = 1 + uleb128_size(payload.len) + payload.len;
  return node;
}

cells_node_t cells_new_value0v(span_byte_t payload) {
  return cells_new_variable((cells_node_type_t){.value = CELLS_NODE_TYPE_VALUEV0}, payload);
}

cells_node_t cells_new_value1v(span_byte_t payload) {
  return cells_new_variable((cells_node_type_t){.value = CELLS_NODE_TYPE_VALUEV1}, payload);
}

cells_node_t cells_new_value2v(span_byte_t payload) {
  return cells_new_variable((cells_node_type_t){.value = CELLS_NODE_TYPE_VALUEV2}, payload);
}

error_t cells_dereference_node(cells_t* cells, size_t* index, cells_node_t* out_node) {
  while (1) {
    cells_node_header_t header = cells_get_node_header(cells, *index);
    if (header.type.value == CELLS_NODE_TYPE_INVALID) { return ERROR_GENERIC; }
    *out_node = cells_get_node(cells, *index, header);
    if (out_node->header.type.value == CELLS_NODE_TYPE_INVALID) { return ERROR_GENERIC; }
    if (cells_node_type_is_ref(header.type)) {
      *index += out_node->as.ref;
      continue;
    }
    return ERROR_SUCCESS;
  }
}

error_t cells_get_left_node(cells_t* cells, size_t* parent_index, cells_node_t* out_node) {
  cells_node_header_t parent_header = cells_get_node_header(cells, *parent_index);
  size_t index = *parent_index + parent_header.encoded_size;
  error_t err = cells_dereference_node(cells, &index, out_node);
  if (err != ERROR_SUCCESS) { return err; }
  *parent_index = index;
  return ERROR_SUCCESS;
}

error_t cells_get_right_node(cells_t* cells, size_t* parent_index, cells_node_t* out_node) {
  cells_node_header_t parent_header = cells_get_node_header(cells, *parent_index);
  cells_node_header_t header =
      cells_get_node_header(cells, *parent_index + parent_header.encoded_size);
  if (header.type.value == CELLS_NODE_TYPE_INVALID) { return ERROR_GENERIC; }
  if (!cells_node_type_is_ref(header.type)) { return ERROR_GENERIC; }

  size_t index = *parent_index + parent_header.encoded_size + header.encoded_size;

  error_t err = cells_dereference_node(cells, &index, out_node);
  if (err != ERROR_SUCCESS) { return err; }
  *parent_index = index;
  return ERROR_SUCCESS;
}
