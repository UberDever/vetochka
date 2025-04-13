#include "common.h"
#include "stb_ds.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define BITS_PER_CELL 2
#define BITS_PER_WORD (sizeof(word_t) * 8)
#define CELLS_PER_WORD (BITS_PER_WORD / BITS_PER_CELL)
#define CELLS_BITMAP_SIZE(cap)                                                 \
  ((cap * CELLS_PER_WORD + BITS_PER_WORD - 1) / BITS_PER_WORD)



typedef struct {
  size_t key;
  size_t value;
} cell_word;

struct Allocator_impl {
  word_t *cells;
  word_t *cells_bitmap;
  size_t cells_capacity;

  cell_word *payload_index;

  word_t *payloads;
};

static inline uint get_bit(const word_t *bitmap, size_t index) {
  size_t word_idx = index / BITS_PER_WORD;
  size_t bit_idx = index % BITS_PER_WORD;
  return (bitmap[word_idx] >> bit_idx) & 1;
}

static inline void set_bit(word_t *bitmap, size_t index, uint value) {
  size_t word_idx = index / BITS_PER_WORD;
  size_t bit_idx = index % BITS_PER_WORD;
  if (value) {
    bitmap[word_idx] |= (1ULL << bit_idx);
  } else {
    bitmap[word_idx] &= ~(1ULL << bit_idx);
  }
}

static uint8_t get_cell_val(const struct Allocator_impl *alloc, size_t index) {
  size_t word_index = index / CELLS_PER_WORD;
  size_t shift = (index % CELLS_PER_WORD) * BITS_PER_CELL;
  return (alloc->cells[word_index] >> shift) & 0x3;
}

static void set_cell_val(struct Allocator_impl *alloc, size_t index,
                         uint8_t cell_value) {
  size_t word_index = index / CELLS_PER_WORD;
  size_t shift = (index % CELLS_PER_WORD) * BITS_PER_CELL;
  alloc->cells[word_index] &= ~((uint64_t)0x3 << shift);
  alloc->cells[word_index] |= ((uint64_t)(cell_value & 0x3)) << shift;
}

static bool index_valid(size_t index, size_t cap) {
  return index / CELLS_PER_WORD < cap;
}

uint eval_cells_init(Allocator *alloc, size_t words_count) {
  Allocator cells = calloc(1, sizeof(struct Allocator_impl));
  if (!cells) {
    return ERROR_VALUE;
  }
  cells->cells = calloc(1, words_count * sizeof(*cells->cells));
  cells->cells_capacity = words_count;
  if (!cells->cells) {
    return ERROR_VALUE;
  }

  cells->cells_bitmap = calloc(1, CELLS_BITMAP_SIZE(cells->cells_capacity) *
                                      sizeof(*cells->cells_bitmap));
  if (!cells->cells_bitmap) {
    return ERROR_VALUE;
  }
  stbds_arrsetcap(cells->payloads, 32);

  *alloc = cells;
  return 0;
}

uint eval_cells_free(Allocator *alloc) {
  Allocator cells = *alloc;
  free(cells->cells);
  free(cells->cells_bitmap);
  stbds_hmfree(cells->payload_index);
  stbds_arrfree(cells->payloads);
  free(cells);
  *alloc = NULL;
  return 0;
}

uint eval_cells_get(Allocator cells, size_t index) {

  if (!index_valid(index, cells->cells_capacity) ||
      !get_bit(cells->cells_bitmap, index)) {
    return ERROR_VALUE;
  }
  return get_cell_val(cells, index);
}

word_t eval_cells_get_word(Allocator cells, size_t index) {
  if (!index_valid(index, cells->cells_capacity) ||
      !get_bit(cells->cells_bitmap, index)) {
    return ERROR_VALUE;
  }
  int64_t pair_idx = stbds_hmgeti(cells->payload_index, index);
  if (pair_idx == -1) {
    return ERROR_VALUE;
  }
  size_t payload_idx = cells->payload_index[pair_idx].value;
  return cells->payloads[payload_idx];
}

uint eval_cells_set(Allocator cells, size_t index, uint8_t value) {
  if (!index_valid(index, cells->cells_capacity)) {
    cells->cells_capacity *= 2;
    cells->cells =
        realloc(cells->cells, cells->cells_capacity * sizeof(*cells->cells));
    if (!cells->cells) {
      return ERROR_VALUE;
    }
    size_t new_cell_bitmap_size = CELLS_BITMAP_SIZE(cells->cells_capacity);
    cells->cells_bitmap =
        realloc(cells->cells_bitmap,
                new_cell_bitmap_size * sizeof(*cells->cells_bitmap));
    if (!cells->cells_bitmap) {
      return ERROR_VALUE;
    }
  }
  set_cell_val(cells, index, value);
  set_bit(cells->cells_bitmap, index, 1);
  return 0;
}

uint eval_cells_set_word(Allocator cells, size_t index, word_t value) {
  if (!index_valid(index, cells->cells_capacity) ||
      !get_bit(cells->cells_bitmap, index)) {
    return ERROR_VALUE;
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

uint eval_cells_is_set(Allocator cells, size_t index) {
  uint result = eval_cells_get(cells, index);
  if (result == ERROR_VALUE) {
    return 0;
  }
  return 1;
}

uint eval_cells_clear(Allocator cells) {
  if (!cells->cells) {
    return ERROR_VALUE;
  }
  memset(cells->cells, 0, cells->cells_capacity * sizeof(*cells->cells));
  if (!cells->cells_bitmap) {
    return ERROR_VALUE;
  }
  memset(cells->cells_bitmap, 0,
         CELLS_BITMAP_SIZE(cells->cells_capacity) *
             sizeof(*cells->cells_bitmap));
  for (size_t i = 0; i < stbds_hmlenu(cells->payload_index); ++i) {
    int res = stbds_hmdel(cells->payload_index, cells->payload_index[i].key);
    assert(res == 1);
  }
  memset(cells->payloads, 0,
         stbds_arrlenu(cells->payloads) * sizeof(*cells->payloads));
  return 0;
}