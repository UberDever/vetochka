
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vendor/stb_ds.h"

#include "memory.h"

#define CELLS_BITMAP_SIZE(cap) BITMAP_SIZE(cap* CELLS_PER_WORD)

uint* _bitmap_init(size_t capacity) {
  return calloc(1, BITMAP_SIZE(capacity) * sizeof(uint));
}

u8 _bitmap_get_bit(const uint* bitmap, size_t index) {
  size_t word_idx = index / BITS_PER_WORD;
  size_t bit_idx = index % BITS_PER_WORD;
  return (bitmap[word_idx] >> bit_idx) & 1;
}

void _bitmap_set_bit(uint* bitmap, size_t index, u8 value) {
  size_t word_idx = index / BITS_PER_WORD;
  size_t bit_idx = index % BITS_PER_WORD;
  if (value) {
    bitmap[word_idx] |= (1ULL << bit_idx);
  } else {
    bitmap[word_idx] &= ~(1ULL << bit_idx);
  }
}

static uint8_t get_cell_val(const struct allocator_t* alloc, size_t index) {
  size_t word_index = index / CELLS_PER_WORD;
  size_t shift = (index % CELLS_PER_WORD) * BITS_PER_CELL;
  return (alloc->cells[word_index] >> shift) & 0x3;
}

static void set_cell_val(struct allocator_t* alloc, size_t index, uint8_t cell_value) {
  size_t word_index = index / CELLS_PER_WORD;
  size_t shift = (index % CELLS_PER_WORD) * BITS_PER_CELL;
  alloc->cells[word_index] &= ~((uint64_t)0x3 << shift);
  alloc->cells[word_index] |= ((uint64_t)(cell_value & 0x3)) << shift;
}

static bool index_valid(size_t index, size_t cap) {
  return index / CELLS_PER_WORD < cap;
}

sint eval_cells_init(allocator_t** alloc, size_t words_count) {
  allocator_t* cells = calloc(1, sizeof(struct allocator_t));
  if (!cells) {
    return ERR_VAL;
  }
  cells->cells = calloc(1, words_count * sizeof(*cells->cells));
  cells->cells_capacity = words_count;
  if (!cells->cells) {
    return ERR_VAL;
  }

  cells->cells_bitmap =
      calloc(1, CELLS_BITMAP_SIZE(cells->cells_capacity) * sizeof(*cells->cells_bitmap));
  if (!cells->cells_bitmap) {
    return ERR_VAL;
  }
  stbds_arrsetcap(cells->payloads, 32);

  *alloc = cells;
  return 0;
}

sint eval_cells_free(allocator_t** alloc) {
  allocator_t* cells = *alloc;
  free(cells->cells);
  free(cells->cells_bitmap);
  stbds_hmfree(cells->payload_index);
  stbds_arrfree(cells->payloads);
  free(cells);
  *alloc = NULL;
  return 0;
}

sint eval_cells_get(allocator_t* cells, size_t index) {
  if (!index_valid(index, cells->cells_capacity) || !_bitmap_get_bit(cells->cells_bitmap, index)) {
    return ERR_VAL;
  }
  return get_cell_val(cells, index);
}

sint eval_cells_get_word(allocator_t* cells, size_t index, sint* word) {
  if (!index_valid(index, cells->cells_capacity) || !_bitmap_get_bit(cells->cells_bitmap, index)) {
    return ERR_VAL;
  }
  int64_t pair_idx = stbds_hmgeti(cells->payload_index, index);
  if (pair_idx == -1) {
    return ERR_VAL;
  }
  size_t payload_idx = cells->payload_index[pair_idx].value;
  *word = cells->payloads[payload_idx];
  return 0;
}

sint eval_cells_set(allocator_t* cells, size_t index, u8 value) {
  if (!index_valid(index, cells->cells_capacity)) {
    cells->cells_capacity *= 2;
    cells->cells = realloc(cells->cells, cells->cells_capacity * sizeof(*cells->cells));
    if (!cells->cells) {
      return ERR_VAL;
    }
    cells->cells_bitmap = realloc(
        cells->cells_bitmap,
        CELLS_BITMAP_SIZE(cells->cells_capacity) * sizeof(*cells->cells_bitmap));
    if (!cells->cells_bitmap) {
      return ERR_VAL;
    }
    return eval_cells_set(cells, index, value);
  }
  set_cell_val(cells, index, value);
  _bitmap_set_bit(cells->cells_bitmap, index, 1);
  return 0;
}

sint eval_cells_set_word(allocator_t* cells, size_t index, sint value) {
  if (!index_valid(index, cells->cells_capacity) || !_bitmap_get_bit(cells->cells_bitmap, index)) {
    return ERR_VAL;
  }
  int64_t pair_idx = stbds_hmgeti(cells->payload_index, index);
  if (pair_idx != -1) {
    size_t word_idx = cells->payload_index[pair_idx].value;
    cells->payloads[word_idx] = value;
    return 0;
  }

  size_t word_idx = stbds_arrlenu(cells->payloads);
  stbds_arrput(cells->payloads, value);
  stbds_hmput(cells->payload_index, index, word_idx);
  return 0;
}

sint eval_cells_is_set(allocator_t* cells, size_t index) {
  sint result = eval_cells_get(cells, index);
  if (result == ERR_VAL) {
    return 0;
  }
  return 1;
}

sint eval_cells_reset(allocator_t* cells) {
  if (!cells) {
    return ERR_VAL;
  }
  if (!cells->cells) {
    return ERR_VAL;
  }
  memset(cells->cells, 0, cells->cells_capacity * sizeof(*cells->cells));
  if (!cells->cells_bitmap) {
    return ERR_VAL;
  }
  memset(
      cells->cells_bitmap,
      0,
      CELLS_BITMAP_SIZE(cells->cells_capacity) * sizeof(*cells->cells_bitmap));
  for (size_t i = 0; i < stbds_hmlenu(cells->payload_index); ++i) {
    int res = stbds_hmdel(cells->payload_index, cells->payload_index[i].key);
    assert(res == 1);
  }
  memset(cells->payloads, 0, stbds_arrlenu(cells->payloads) * sizeof(*cells->payloads));
  return 0;
}
