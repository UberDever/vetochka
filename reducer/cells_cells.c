#include "cells_api.h"
#include "cells_impl.h"
#include "typedefs.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

error_t cells_create(struct cells_t** cells, size_t capacity) {
  *cells = calloc(1, sizeof(struct cells_t));
  (*cells)->data = calloc(1, capacity);
  if (!(*cells)->data) { return ERROR_GENERIC; }
  (*cells)->occupied_bitmap = _bitmap_alloc(capacity);
  if (!(*cells)->occupied_bitmap) {
    free((*cells)->data);
    return ERROR_GENERIC;
  }
  (*cells)->capacity = capacity;
  return ERROR_SUCCESS;
}

void cells_destroy(struct cells_t** cells) {
  free((*cells)->data);
  free((*cells)->occupied_bitmap);
  free(*cells);
}

static inline byte tag_from_type(enum CELLS_NODE_TYPE type) {
  assert(type != CELLS_NODE_TYPE_INVALID);
  // we just encode all nodes as tag + payload, so mapping from type to tag is straightforward
  return 0x80 - 1 + (byte)type;
}

static inline u16 read_u16_be(const byte* p) {
  return ((u16)p[0] << 8) | (u16)p[1];
}

static inline void write_u16_be(byte* p, u16 v) {
  p[0] = (byte)(v >> 8);
  p[1] = (byte)(v & 0xffU);
}

static inline u64 read_u64_be(const byte* p) {
  u64 v = 0;
  for (size_t i = 0; i < sizeof(v); i++) {
    v = (v << 8) | (u64)p[i];
  }
  return v;
}

static inline void write_u64_be(byte* p, u64 v) {
  for (size_t i = 0; i < sizeof(v); i++) {
    p[sizeof(v) - 1 - i] = (byte)(v & 0xffU);
    v >>= 8;
  }
}

struct cells_node_meta_t cells_get_node_meta(struct cells_t* cells, size_t index) {
  struct cells_node_meta_t meta = {0};
  if (index > cells->capacity) { return meta; }
  byte b = cells->data[index];

  if ((b >> 6) == 0x00) {
    meta.type = CELLS_NODE_TYPE_REF2;
    meta.size = sizeof(i16);
    goto check_occupied;
  }
  if ((b >> 6) == 0x01) {
    meta.type = CELLS_NODE_TYPE_REF8;
    meta.size = sizeof(i64);
    goto check_occupied;
  }

#define CASE_TAG(t, sz)                                                                            \
  if (b == tag_from_type(CELLS_NODE_TYPE_##t)) {                                                   \
    meta.type = CELLS_NODE_TYPE_##t;                                                               \
    meta.size = (sz);                                                                              \
    goto check_occupied;                                                                           \
  }

  CASE_TAG(DELTA0, 1);
  CASE_TAG(DELTA1, 1);
  CASE_TAG(DELTA2, 1);
  CASE_TAG(VALUEF0, 9);
  CASE_TAG(VALUEF1, 9);
  CASE_TAG(VALUEF2, 9);

  bool is_valuev = false;
  if (b == tag_from_type(CELLS_NODE_TYPE_VALUEV0)) {
    is_valuev = true;
    meta.type = CELLS_NODE_TYPE_VALUEV0;
  }
  if (b == tag_from_type(CELLS_NODE_TYPE_VALUEV1)) {
    is_valuev = true;
    meta.type = CELLS_NODE_TYPE_VALUEV1;
  }
  if (b == tag_from_type(CELLS_NODE_TYPE_VALUEV2)) {
    is_valuev = true;
    meta.type = CELLS_NODE_TYPE_VALUEV2;
  }
  if (is_valuev) {
    if (index + 1 > cells->capacity) { return meta; }
    size_t uleb_len = 0;
    size_t uleb_val = 0;
    if (uleb128_read(cells->data, cells->capacity, index + 1, &uleb_len, &uleb_val) != 0) {
      return (struct cells_node_meta_t){0};
    }
    meta.size = 1 + uleb_len + uleb_val;
    goto check_occupied;
  }

  CASE_TAG(SEQ0, 1);
  CASE_TAG(SEQ1, 1);
  CASE_TAG(SEQ2, 1);
  CASE_TAG(CALL0, 1);
  CASE_TAG(CALL1, 1);
  CASE_TAG(CALL2, 1);

#undef CASE_TAG

check_occupied:
  for (size_t i = index; i < index + meta.size; i++) {
    if (!_bitmap_get_bit(cells->occupied_bitmap, i)) {
      meta.type = CELLS_NODE_TYPE_INVALID;
      meta.size = 0;
      return meta;
    }
  }

  return meta;
}

struct cells_node_t cells_get_node(
    struct cells_t* cells, size_t index, struct cells_node_meta_t meta) {
  struct cells_node_t node = {0};
  if (index > cells->capacity) { return node; }

  switch (meta.type) {
    case CELLS_NODE_TYPE_REF2: {
      i16 offset = 0;
      if ((cells->data[index] >> 6) != 0x00) { return node; }
      if (index + sizeof(offset) > cells->capacity) { return node; }
      u16 raw = read_u16_be(cells->data + index);
      offset = (i16)(raw & 0x3fffU);
      if ((offset & 0x2000) != 0) { offset |= (i16)0xc000; }
      node.meta = meta;
      node.as.ref = offset;
      return node;
    }

    case CELLS_NODE_TYPE_REF8: {
      i64 offset = 0;
      if ((cells->data[index] >> 6) != 0x01) { return node; }
      if (index + sizeof(offset) > cells->capacity) { return node; }
      u64 raw = read_u64_be(cells->data + index);
      offset = (i64)(raw & UINT64_C(0x3fffffffffffffff));
      if ((offset & (i64)UINT64_C(0x2000000000000000)) != 0) {
        offset |= (i64)UINT64_C(0xc000000000000000);
      }
      node.meta = meta;
      node.as.ref = offset;
      return node;
    }
    case CELLS_NODE_TYPE_DELTA0:
    case CELLS_NODE_TYPE_DELTA1:
    case CELLS_NODE_TYPE_DELTA2: {
      node.meta = meta;
      return node;
    }
    case CELLS_NODE_TYPE_VALUEF0: {
      if (tag_from_type(CELLS_NODE_TYPE_VALUEF0) != cells->data[index]) { return node; }
      goto handle_valuef;
    }
    case CELLS_NODE_TYPE_VALUEF1: {
      if (tag_from_type(CELLS_NODE_TYPE_VALUEF1) != cells->data[index]) { return node; }
      goto handle_valuef;
    }
    case CELLS_NODE_TYPE_VALUEF2: {
      if (tag_from_type(CELLS_NODE_TYPE_VALUEF2) != cells->data[index]) { return node; }
      i64 value = 0;
    handle_valuef:
      if (index + sizeof(value) + 1 > cells->capacity) { return node; }
      memcpy(&value, cells->data + index + 1, sizeof(i64));
      node.meta = meta;
      node.as.nativef = value;
      return node;
    }
    case CELLS_NODE_TYPE_VALUEV0: {
      if (tag_from_type(CELLS_NODE_TYPE_VALUEV0) != cells->data[index]) { return node; }
      goto handle_valuev;
    }
    case CELLS_NODE_TYPE_VALUEV1: {
      if (tag_from_type(CELLS_NODE_TYPE_VALUEV1) != cells->data[index]) { return node; }
      goto handle_valuev;
    }
    case CELLS_NODE_TYPE_VALUEV2: {
      if (tag_from_type(CELLS_NODE_TYPE_VALUEV2) != cells->data[index]) { return node; }
      struct span_byte_t payload = {0};
      size_t uleb_len = 0;
      size_t uleb_val = 0;
    handle_valuev:
      if (uleb128_read(cells->data, cells->capacity, index + 1, &uleb_len, &uleb_val) != 0) {
        return node;
      }
      if (index + uleb_len + uleb_val + 1 > cells->capacity) { return node; }
      payload.len = uleb_val;
      payload.data = cells->data + index + 1 + uleb_len;
      node.as.nativev = payload;
      node.meta = meta;
      return node;
    }
    case CELLS_NODE_TYPE_SEQ0:
    case CELLS_NODE_TYPE_SEQ1:
    case CELLS_NODE_TYPE_SEQ2:
    case CELLS_NODE_TYPE_CALL0:
    case CELLS_NODE_TYPE_CALL1:
    case CELLS_NODE_TYPE_CALL2: {
      node.meta = meta;
      return node;
    }
    case CELLS_NODE_TYPE_INVALID:
      // TODO: report here, stacktrace + node data
      return node;
  }
  return node;
}

/**
 * Try to allocate a chunk of memory with the specified size and get its index.
 * Does not mark the cells as occupied.
 * @return index of the possibly allocated chunk, or 0 on failure.
 */
static error_t cells_try_alloc_chunk(struct cells_t* cells, size_t chunk_size, size_t* index_out) {
  size_t start_index = cells->next_free_index % (cells->capacity / 2);
  size_t free_bytes_count = 0;
  for (size_t i = start_index; i < cells->capacity; i++) {
    if (!_bitmap_get_bit(cells->occupied_bitmap, i)) {
      free_bytes_count++;
    } else {
      free_bytes_count = 0;
    }
    if (free_bytes_count == chunk_size) {
      *index_out = i - free_bytes_count + 1;
      return ERROR_SUCCESS;
    }
  }
  return ERROR_GENERIC;
}

error_t cells_alloc_chunk(struct cells_t* cells, size_t chunk_size, size_t* index_out) {
  if (chunk_size == 0) { return ERROR_SUCCESS; }
  // to keep next index close to the start of the memory, cap it to part of capacity
  error_t err = cells_try_alloc_chunk(cells, chunk_size, index_out);
  if (err != ERROR_SUCCESS) { return err; }
  for (size_t j = *index_out; j < *index_out + chunk_size; j++) {
    _bitmap_set_bit(cells->occupied_bitmap, j, 1);
  }
  cells->next_free_index = *index_out + chunk_size;
  return ERROR_SUCCESS;
}

error_t cells_write_node(struct cells_t* cells, size_t index, struct cells_node_t node) {
  switch (node.meta.type) {
    case CELLS_NODE_TYPE_REF2: {
      u16 offset = 0;
      if (index + sizeof(offset) > cells->capacity) { return ERROR_OUT_OF_BOUNDS; }
      if (node.as.ref > 0x1fff || node.as.ref < -0x1fff) { return ERROR_INVALID_PARAM; }
      offset = (u16)(0x00 << 14) | (u16)(node.as.ref & 0x3fff);
      write_u16_be(cells->data + index, offset);
      return ERROR_SUCCESS;
    }
    case CELLS_NODE_TYPE_REF8: {
      u64 offset = 0;
      if (index + sizeof(offset) > cells->capacity) { return ERROR_OUT_OF_BOUNDS; }
      if (node.as.ref > 0x1fffffffffffffff || node.as.ref < -0x1fffffffffffffff) {
        return ERROR_INVALID_PARAM;
      }
      offset = (u64)(0x01LU << 62) | (u64)(node.as.ref & 0x3fffffffffffffffLU);
      write_u64_be(cells->data + index, offset);
      return ERROR_SUCCESS;
    }

#define CASE_WRITE_TAG(t)                                                                          \
  case CELLS_NODE_TYPE_##t: {                                                                      \
    if (index + 1 > cells->capacity) { return ERROR_OUT_OF_BOUNDS; }                               \
    cells->data[index] = tag_from_type(CELLS_NODE_TYPE_##t);                                       \
    return ERROR_SUCCESS;                                                                          \
  }

      CASE_WRITE_TAG(DELTA0)
      CASE_WRITE_TAG(DELTA1)
      CASE_WRITE_TAG(DELTA2)

    case CELLS_NODE_TYPE_VALUEF0:
    case CELLS_NODE_TYPE_VALUEF1:
    case CELLS_NODE_TYPE_VALUEF2:
      if (index + 1 + sizeof(i64) > cells->capacity) { return ERROR_OUT_OF_BOUNDS; }
      cells->data[index] = tag_from_type(node.meta.type);
      memcpy(cells->data + index + 1, &node.as.nativef, sizeof(i64));
      return ERROR_SUCCESS;

    case CELLS_NODE_TYPE_VALUEV0:
    case CELLS_NODE_TYPE_VALUEV1:
    case CELLS_NODE_TYPE_VALUEV2: {
      if (index + 1 > cells->capacity) { return ERROR_OUT_OF_BOUNDS; }
      cells->data[index] = tag_from_type(node.meta.type);
      size_t uleb_len = uleb128_write(cells->data + index + 1, node.as.nativev.len);
      if (index + 1 + uleb_len + node.as.nativev.len > cells->capacity) {
        return ERROR_OUT_OF_BOUNDS;
      }
      memcpy(cells->data + index + 1 + uleb_len, node.as.nativev.data, node.as.nativev.len);
      return ERROR_SUCCESS;
    }

      CASE_WRITE_TAG(SEQ0)
      CASE_WRITE_TAG(SEQ1)
      CASE_WRITE_TAG(SEQ2)
      CASE_WRITE_TAG(CALL0)
      CASE_WRITE_TAG(CALL1)
      CASE_WRITE_TAG(CALL2)

    case CELLS_NODE_TYPE_INVALID:
      // TODO: report here, stacktrace + node data
      assert(0 && "invalid node");
  }

#undef CASE_WRITE_TAG
}

error_t cells_node_free(struct cells_t* cells, size_t index, size_t node_size) {
  for (size_t i = index; i < index + node_size; ++i) {
    cells->data[i] = 0;
    _bitmap_set_bit(cells->occupied_bitmap, i, 0);
  }
  cells->next_free_index = index % (cells->capacity / 2);
  return ERROR_SUCCESS;
}
