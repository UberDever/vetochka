#include "common.h"
#include <stdlib.h>
#include <string.h>

// Define cell and word sizes in bits
#define CELL_SIZE 2
#define WORD_SIZE (sizeof(word_t) * 8)
#define CELLS_IN_WORD (WORD_SIZE / CELL_SIZE)
#define CELL_MASK_SIZE 5

// Index encoding masks
#define MSB_MASK                                                               \
  (1ULL << (WORD_SIZE - 1)) // MSB to indicate type (0 = cell, 1 = word)
#define CELL_INDEX_MASK (CELLS_IN_WORD - 1) // Bits 0-4 for cell index (0-31)
#define WORD_INDEX_MASK 0x3FFFFFFFFFFFFF    // Bits 5-62 for word index

typedef uint64_t word_t;

static inline word_t get_bit(const word_t *bitmap, size_t index) {
  size_t word_idx = index / WORD_SIZE;
  size_t bit_idx = index % WORD_SIZE;
  return (bitmap[word_idx] >> bit_idx) & 1;
}

static inline void set_bit(word_t *bitmap, size_t index, word_t value) {
  size_t word_idx = index / WORD_SIZE;
  size_t bit_idx = index % WORD_SIZE;
  if (value) {
    bitmap[word_idx] |= (1ULL << bit_idx);
  } else {
    bitmap[word_idx] &= ~(1ULL << bit_idx);
  }
}

struct Allocator_impl {
  word_t *words;           // Array of 64-bit words
  size_t words_size;       // Number of 64-bit words in the pool
  word_t *type_bitmap;     // Bitmap: 0 = word used for cells, 1 = used for a
                           // data word
  size_t type_bitmap_size; // Number of uint64_t in word_type_bitmap
  word_t *cell_bitmap; // Bitmap: allocation status of each cell (total_words
                       // * 32 bits)
  size_t cell_bitmap_size; // Number of uint64_t in cell_bitmap
};

uint eval_cells_init(struct Allocator_impl **cells, size_t initial_words) {
  *cells = malloc(sizeof(struct Allocator_impl));
  (*cells)->words_size = initial_words;
  (*cells)->words = malloc(initial_words * sizeof(word_t));
  memset((*cells)->words, 0, initial_words * sizeof(word_t));

  (*cells)->type_bitmap_size = (initial_words + WORD_SIZE - 1) / WORD_SIZE;
  (*cells)->type_bitmap = malloc((*cells)->type_bitmap_size * sizeof(word_t));
  memset((*cells)->type_bitmap, 0, (*cells)->type_bitmap_size * sizeof(word_t));

  (*cells)->cell_bitmap_size =
      (initial_words * CELLS_IN_WORD + WORD_SIZE - 1) / WORD_SIZE;
  (*cells)->cell_bitmap = malloc((*cells)->cell_bitmap_size * sizeof(word_t));
  memset((*cells)->cell_bitmap, 0, (*cells)->cell_bitmap_size * sizeof(word_t));
  return 0;
}

uint eval_cells_free(struct Allocator_impl **cells) {
  free((*cells)->words);
  free((*cells)->type_bitmap);
  free((*cells)->cell_bitmap);
  (*cells)->words_size = 0;
  (*cells)->type_bitmap_size = 0;
  (*cells)->cell_bitmap_size = 0;
  free(*cells);
  *cells = NULL;
  return 0;
}

static void grow_pool(struct Allocator_impl *cells) {
  size_t old_total_words = cells->words_size;
  size_t new_total_words = old_total_words * 2;

  cells->words = realloc(cells->words, new_total_words * sizeof(word_t));
  memset(cells->words + old_total_words, 0,
         (new_total_words - old_total_words) * sizeof(word_t));

  size_t new_word_type_size = (new_total_words + WORD_SIZE - 1) / WORD_SIZE;
  cells->type_bitmap =
      realloc(cells->type_bitmap, new_word_type_size * sizeof(word_t));
  memset(cells->type_bitmap + cells->type_bitmap_size, 0,
         (new_word_type_size - cells->type_bitmap_size) * sizeof(word_t));
  cells->type_bitmap_size = new_word_type_size;

  size_t new_cell_bitmap_size =
      (new_total_words * CELLS_IN_WORD + WORD_SIZE - 1) / WORD_SIZE;
  cells->cell_bitmap =
      realloc(cells->cell_bitmap, new_cell_bitmap_size * sizeof(word_t));
  memset(cells->cell_bitmap + cells->cell_bitmap_size, 0,
         (new_cell_bitmap_size - cells->cell_bitmap_size) * sizeof(word_t));
  cells->cell_bitmap_size = new_cell_bitmap_size;

  cells->words_size = new_total_words;
}

uint eval_cells_new_cell(struct Allocator_impl *cells) {
  for (size_t i = 0; i < cells->words_size; i++) {
    if (get_bit(cells->type_bitmap, i) == 0) { // Word used for cells
      for (size_t k = 0; k < CELLS_IN_WORD; k++) {
        size_t cell_bit = (i * CELLS_IN_WORD) + k;
        if (get_bit(cells->cell_bitmap, cell_bit) == 0) {
          set_bit(cells->cell_bitmap, cell_bit, 1);
          return (i << CELL_MASK_SIZE) | k; // MSB = 0, word index, cell index
        }
      }
    }
  }
  grow_pool(cells);
  return eval_cells_new_cell(cells);
}

uint eval_cells_new_word(struct Allocator_impl *cells) {
  for (size_t i = 0; i < cells->words_size; i++) {
    int all_free = 1;
    for (size_t k = 0; k < CELLS_IN_WORD; k++) {
      if (get_bit(cells->cell_bitmap, (i * CELLS_IN_WORD) + k) != 0) {
        all_free = 0;
        break;
      }
    }
    if (all_free) {
      set_bit(cells->type_bitmap, i, 1);
      for (size_t k = 0; k < CELLS_IN_WORD; k++) {
        set_bit(cells->cell_bitmap, (i * CELLS_IN_WORD) + k, 1);
      }
      return MSB_MASK | (i << CELL_MASK_SIZE); // MSB = 1, word index
    }
  }
  grow_pool(cells);
  return eval_cells_new_word(cells);
}

uint eval_cells_next_cell(struct Allocator_impl *cells, uint index) {
  if (index & MSB_MASK) {
    return (uint)-1; // Input must be a cell index
  }
  size_t w = (index >> CELL_MASK_SIZE) & WORD_INDEX_MASK; // Word index
  size_t c = index & CELL_INDEX_MASK;                     // Cell index

  if (c < CELLS_IN_WORD - 1) {
    return (w << CELL_MASK_SIZE) | (c + 1); // Next cell in the same word
  }
  // Find the next word used for cells
  for (size_t j = w + 1; j < cells->words_size; j++) {
    if ((get_bit(cells->type_bitmap, j) == 0) &&
        (get_bit(cells->cell_bitmap, j * CELLS_IN_WORD) == 1)) {
      return (j << CELL_MASK_SIZE) | 0; // First cell in the next cell word
    }
  }
  return (uint)-1; // No next cell found
}

uint eval_cells_next_word(struct Allocator_impl *cells, uint index) {
  if (index & MSB_MASK) {
    return (uint)-1; // Input must be a cell index
  }
  size_t w = (index >> CELL_MASK_SIZE) & WORD_INDEX_MASK; // Word index

  // Find the next word used for a data word
  for (size_t j = w + 1; j < cells->words_size; j++) {
    if (get_bit(cells->type_bitmap, j) == 1) {
      return MSB_MASK | (j << CELL_MASK_SIZE); // Next word index
    }
  }
  return (uint)-1; // No next word found
}

uint eval_cells_delete(struct Allocator_impl *cells, uint index) {
  if (index & MSB_MASK) { // Word index
    size_t w = (index >> CELL_MASK_SIZE) & WORD_INDEX_MASK;
    if (w >= cells->words_size || !get_bit(cells->type_bitmap, w)) {
      return 0; // Invalid or not allocated
    }
    set_bit(cells->type_bitmap, w, 0);
    for (size_t k = 0; k < CELLS_IN_WORD; k++) {
      set_bit(cells->cell_bitmap, (w * CELLS_IN_WORD) + k, 0);
    }
    return 1;
  }
  // Cell index
  size_t w = (index >> CELL_MASK_SIZE) & WORD_INDEX_MASK;
  size_t c = index & CELL_INDEX_MASK;
  if (w >= cells->words_size || c >= CELLS_IN_WORD ||
      get_bit(cells->type_bitmap, w)) {
    return 0; // Invalid or word is not used for cells
  }
  size_t cell_bit = (w * CELLS_IN_WORD) + c;
  if (!get_bit(cells->cell_bitmap, cell_bit)) {
    return 0; // Not allocated
  }
  set_bit(cells->cell_bitmap, cell_bit, 0);
  return 1;
}

uint eval_cells_get(struct Allocator_impl *cells, uint index) {
  if (index & MSB_MASK) { // Word
    size_t w = (index >> CELL_MASK_SIZE) & WORD_INDEX_MASK;
    return cells->words[w]; // No bounds checking as per note
  }
  // Cell
  size_t w = (index >> CELL_MASK_SIZE) & WORD_INDEX_MASK;
  size_t c = index & CELL_INDEX_MASK;
  word_t word = cells->words[w];
  return (word >> (c * CELL_SIZE)) & 0x3; // Extract 2-bit cell value
}

uint eval_cells_is_set(struct Allocator_impl *cells, uint index) {
  if (index & MSB_MASK) { // Word
    size_t w = (index >> CELL_MASK_SIZE) & WORD_INDEX_MASK;
    if (w >= cells->words_size) {
      return (uint)-1;
    }
    return get_bit(cells->type_bitmap, w);
  }
  // Cell
  size_t w = (index >> CELL_MASK_SIZE) & WORD_INDEX_MASK;
  size_t c = index & CELL_INDEX_MASK;
  if (w >= cells->words_size || c >= CELLS_IN_WORD) {
    return (uint)-1;
  }
  if (get_bit(cells->type_bitmap, w)) {
    return 0; // Word used for a data word
  }
  return get_bit(cells->cell_bitmap,
                 (w * CELLS_IN_WORD) + c); // 1 if allocated, 0 if not
}

uint eval_cells_set(struct Allocator_impl *cells, uint index, uint value) {
  if (index & MSB_MASK) { // Word index
    size_t w = (index >> CELL_MASK_SIZE) & WORD_INDEX_MASK;
    if (w >= cells->words_size || !get_bit(cells->type_bitmap, w)) {
      return 0;
    }
    cells->words[w] = value;
    return 1;
  }
  // Cell index
  size_t w = (index >> CELL_MASK_SIZE) & WORD_INDEX_MASK;
  size_t c = index & CELL_INDEX_MASK;
  if (w >= cells->words_size || c >= CELLS_IN_WORD ||
      get_bit(cells->type_bitmap, w)) {
    return 0; // Invalid or word is not used for cells
  }
  if (!get_bit(cells->cell_bitmap, (w * CELLS_IN_WORD) + c)) {
    return 0; // Not allocated
  }
  // Clear the 2 bits and set the new value
  word_t *word = &cells->words[w];
  *word = (*word & ~(0x3ULL << (c * CELL_SIZE))) |
          ((value & 0x3) << (c * CELL_SIZE));
  return 1;
}
