#ifndef __REDUCER_CELLS_IMPL_H__
#define __REDUCER_CELLS_IMPL_H__

#include "cells_api.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

enum cells_node_layout_t {
  CELLS_NODE_LAYOUT_INVALID,
  CELLS_NODE_LAYOUT_TAG,
  CELLS_NODE_LAYOUT_I64,
  CELLS_NODE_LAYOUT_BYTES,
  CELLS_NODE_LAYOUT_REF14,
  CELLS_NODE_LAYOUT_REF62,
};

/*
 * X(WIRE, TYPE, MASK, CODE, LAYOUT, ARITY, NEXT_TYPE, FIXED_ENCODED_SIZE)
 *
 * MASK/CODE match first byte: (byte & MASK) == CODE.
 * REF14 and REF62 intentionally map to same semantic REF type.
 * Published MASK/CODE/layout combinations form stable bytecode ABI.
 */
#define CELLS_NODE_INFO_ITEMS(X)                                                                   \
  X(DELTA0, DELTA0, 0xFF, 0x80, TAG, 0, DELTA1, 1)                                                 \
  X(DELTA1, DELTA1, 0xFF, 0x81, TAG, 1, DELTA2, 1)                                                 \
  X(DELTA2, DELTA2, 0xFF, 0x82, TAG, 2, INVALID, 1)                                                \
  X(VALUEF0, VALUEF0, 0xFF, 0x83, I64, 0, VALUEF1, 9)                                              \
  X(VALUEF1, VALUEF1, 0xFF, 0x84, I64, 1, VALUEF2, 9)                                              \
  X(VALUEF2, VALUEF2, 0xFF, 0x85, I64, 2, INVALID, 9)                                              \
  X(VALUEV0, VALUEV0, 0xFF, 0x86, BYTES, 0, VALUEV1, 0)                                            \
  X(VALUEV1, VALUEV1, 0xFF, 0x87, BYTES, 1, VALUEV2, 0)                                            \
  X(VALUEV2, VALUEV2, 0xFF, 0x88, BYTES, 2, INVALID, 0)                                            \
  X(REF14, REF, 0xC0, 0x00, REF14, CELLS_NODE_ARITY_NONE, INVALID, 2)                              \
  X(REF62, REF, 0xC0, 0x40, REF62, CELLS_NODE_ARITY_NONE, INVALID, 8)

MUH_PRIVATE enum cells_node_layout_t cells_node_type_get_layout(cells_node_type_t type);
MUH_PRIVATE size_t cells_node_type_get_fixed_encoded_size(cells_node_type_t type);
MUH_PRIVATE bool cells_ref_fits_ref14(i64 value);
MUH_PRIVATE bool cells_ref_fits_ref62(i64 value);

#define CELL_SIZE_BITS 8
#define BITMAP_SIZE(x) (((x) + CELL_SIZE_BITS - 1) / CELL_SIZE_BITS)

static inline u64* _bitmap_alloc(size_t capacity) {
  return calloc(1, BITMAP_SIZE(capacity) * sizeof(u64));
}

static inline bool _bitmap_get_bit(const u64* bitmap, size_t index) {
  size_t word_idx = index / CELL_SIZE_BITS;
  size_t bit_idx = index % CELL_SIZE_BITS;
  return (bitmap[word_idx] >> bit_idx) & 1;
}

static inline void _bitmap_set_bit(u64* bitmap, size_t index, bool value) {
  size_t word_idx = index / CELL_SIZE_BITS;
  size_t bit_idx = index % CELL_SIZE_BITS;
  if (value) {
    bitmap[word_idx] |= (1ULL << bit_idx);
  } else {
    bitmap[word_idx] &= ~(1ULL << bit_idx);
  }
}

typedef struct cells_t {
  byte* data;
  u64* occupied_bitmap;
  struct cells_free_chunk_t* free_chunks_head;
  size_t capacity;
} cells_t;

struct cells_free_chunk_t {
  size_t index;
  size_t size;
  struct cells_free_chunk_t* next;
};

MUH_UNUSED static inline error_t uleb128_read(
    const byte* data, size_t capacity, size_t index, size_t* uleb_len, size_t* uleb_value) {
  size_t shift = 0;
  while (1) {
    if (index >= capacity) { return ERROR_OUT_OF_BOUNDS; }
    byte b = data[index];
    size_t chunk = b & 0x7f;

    // overflow / shift guard
    if (shift > 8 * sizeof(size_t)) { return ERROR_OVERFLOW; }
    // also guard that shifting chunk won't overflow (conservative)
    if (chunk != 0 && shift > 8 * sizeof(size_t) - 7) { return ERROR_OVERFLOW; }

    *uleb_value |= chunk << shift;
    shift += 7;
    *uleb_len += 1;
    index++;

    if ((b & 0x80) == 0) { break; }
    if (*uleb_len > 9) { assert(0 && "uleb_len > 9"); }
  }
  return 0;
}

MUH_UNUSED static size_t uleb128_size(u64 x) {
  size_t n = 1;
  while (x >= 0x80) {
    x >>= 7;
    n++;
  }
  return n;
}

// writes ULEB128 for x into out, returns bytes written (>=1)
MUH_UNUSED static size_t uleb128_write(byte* out, u64 x) {
  size_t n = 0;
  do {
    byte b = (byte)(x & 0x7F);
    x >>= 7;
    if (x) { b |= 0x80; }
    out[n++] = b;
  } while (x);
  return n;
}

#endif // __REDUCER_CELLS_IMPL_H__
