#ifndef __REDUCER_CELLS_IMPL_H__
#define __REDUCER_CELLS_IMPL_H__

#include "cells_api.h"
#include <stddef.h>
#include <stdlib.h>

#define CELL_SIZE_BITS 8
#define BITMAP_SIZE(x) (((x) + CELL_SIZE_BITS - 1) / CELL_SIZE_BITS)

static u64* _bitmap_alloc(size_t capacity) {
  return calloc(1, BITMAP_SIZE(capacity) * sizeof(u64));
}

static byte _bitmap_get_bit(const u64* bitmap, size_t index) {
  size_t word_idx = index / CELL_SIZE_BITS;
  size_t bit_idx = index % CELL_SIZE_BITS;
  return (bitmap[word_idx] >> bit_idx) & 1;
}

static void _bitmap_set_bit(u64* bitmap, size_t index, byte value) {
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
  u64 capacity;
} cells_t;

#endif // __REDUCER_CELLS_IMPL_H__
