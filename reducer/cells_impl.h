#ifndef __REDUCER_CELLS_IMPL_H__
#define __REDUCER_CELLS_IMPL_H__

#include "cells_api.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

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

__attribute__((unused)) static inline error_t uleb128_read(
    const byte* data, size_t capacity, size_t index, size_t* uleb_len, size_t* uleb_value) {
  size_t shift = 0;
  while (1) {
    if (index > capacity) { return ERROR_OUT_OF_BOUNDS; }
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

__attribute__((unused)) static size_t uleb128_size(u64 x) {
  size_t n = 1;
  while (x >= 0x80) {
    x >>= 7;
    n++;
  }
  return n;
}

// writes ULEB128 for x into out, returns bytes written (>=1)
__attribute__((unused)) static size_t uleb128_write(byte* out, u64 x) {
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
