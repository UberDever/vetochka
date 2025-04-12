#include "common.h"
#include <stdlib.h>
#include <string.h>

// Define cell and word sizes in bits
#define CELL_SIZE 2
#define WORD_SIZE (sizeof(word_t) * 8)

// Index encoding masks
#define MSB_MASK (1ULL << 63) // MSB to indicate type (0 = cell, 1 = word)
#define CELL_INDEX_MASK 0x1F  // Bits 0-4 for cell index (0-31)
#define WORD_INDEX_MASK 0x3FFFFFFFFFFFFF // Bits 5-62 for word index

/* typedef uint64_t word_t; */
/*  */
/* typedef struct { */
/*   word_t *pool;               // Array of 64-bit words */
/*   size_t total_words;         // Number of 64-bit words in the pool */
/*   uint64_t *word_type_bitmap; // Bitmap: 0 = word used for cells, 1 = used for a */
/*                               // data word */
/*   size_t word_type_bitmap_size; // Number of uint64_t in word_type_bitmap */
/*   uint64_t *cell_bitmap; // Bitmap: allocation status of each cell (total_words */
/*                          // * 32 bits) */
/*   size_t cell_bitmap_size; // Number of uint64_t in cell_bitmap */
/* } Allocator; */

// Helper function to get a bit from a bitmap
static inline uint64_t get_bit(uint64_t *bitmap, size_t index) {
  size_t word_idx = index / 64;
  size_t bit_idx = index % 64;
  return (bitmap[word_idx] >> bit_idx) & 1;
}

// Helper function to set a bit in a bitmap
static inline void set_bit(uint64_t *bitmap, size_t index, uint64_t value) {
  size_t word_idx = index / 64;
  size_t bit_idx = index % 64;
  if (value) {
    bitmap[word_idx] |= (1ULL << bit_idx);
  } else {
    bitmap[word_idx] &= ~(1ULL << bit_idx);
  }
}

// Helper function to grow the pool when full
static void grow_pool(Allocator *allocator) {
  size_t old_total_words = allocator->total_words;
  size_t new_total_words = old_total_words * 2;

  // Reallocate pool
  allocator->pool = realloc(allocator->pool, new_total_words * sizeof(word_t));
  memset(allocator->pool + old_total_words, 0,
         (new_total_words - old_total_words) * sizeof(word_t));

  // Reallocate word_type_bitmap
  size_t new_word_type_size = (new_total_words + 63) / 64;
  allocator->word_type_bitmap = realloc(allocator->word_type_bitmap,
                                        new_word_type_size * sizeof(uint64_t));
  memset(allocator->word_type_bitmap + allocator->word_type_bitmap_size, 0,
         (new_word_type_size - allocator->word_type_bitmap_size) *
             sizeof(uint64_t));
  allocator->word_type_bitmap_size = new_word_type_size;

  // Reallocate cell_bitmap
  size_t new_cell_bitmap_size = (new_total_words * 32 + 63) / 64;
  allocator->cell_bitmap =
      realloc(allocator->cell_bitmap, new_cell_bitmap_size * sizeof(uint64_t));
  memset(allocator->cell_bitmap + allocator->cell_bitmap_size, 0,
         (new_cell_bitmap_size - allocator->cell_bitmap_size) *
             sizeof(uint64_t));
  allocator->cell_bitmap_size = new_cell_bitmap_size;

  allocator->total_words = new_total_words;
}

// Allocates a cell or a word in the pool
uint allocator_allocate(Allocator *allocator, uint type) {
  if (type == 0) { // Allocate a cell
    for (size_t i = 0; i < allocator->total_words; i++) {
      if (get_bit(allocator->word_type_bitmap, i) == 0) { // Word used for cells
        for (size_t k = 0; k < 32; k++) {
          size_t cell_bit = i * 32 + k;
          if (get_bit(allocator->cell_bitmap, cell_bit) == 0) {
            set_bit(allocator->cell_bitmap, cell_bit, 1);
            return (i << 5) | k; // MSB = 0, word index, cell index
          }
        }
      }
    }
  } else { // Allocate a word
    for (size_t i = 0; i < allocator->total_words; i++) {
      int all_free = 1;
      for (size_t k = 0; k < 32; k++) {
        if (get_bit(allocator->cell_bitmap, i * 32 + k) != 0) {
          all_free = 0;
          break;
        }
      }
      if (all_free) {
        set_bit(allocator->word_type_bitmap, i, 1);
        for (size_t k = 0; k < 32; k++) {
          set_bit(allocator->cell_bitmap, i * 32 + k, 1);
        }
        return MSB_MASK | (i << 5); // MSB = 1, word index
      }
    }
  }

  // No space available, grow the pool and retry
  grow_pool(allocator);
  return allocator_allocate(allocator, type);
}

// Get the next cell after the provided cell index
uint allocator_next_cell(Allocator *allocator, uint index) {
  if (index & MSB_MASK)
    return (uint)-1;                         // Input must be a cell index
  size_t w = (index >> 5) & WORD_INDEX_MASK; // Word index
  size_t c = index & CELL_INDEX_MASK;        // Cell index

  if (c < 31) {
    return (w << 5) | (c + 1); // Next cell in the same word
  } else {
    // Find the next word used for cells
    for (size_t j = w + 1; j < allocator->total_words; j++) {
      if (get_bit(allocator->word_type_bitmap, j) == 0) {
        return (j << 5) | 0; // First cell in the next cell word
      }
    }
    return (uint)-1; // No next cell found
  }
}

// Get the next word after the provided cell index
uint allocator_next_word(Allocator *allocator, uint index) {
  if (index & MSB_MASK)
    return (uint)-1;                         // Input must be a cell index
  size_t w = (index >> 5) & WORD_INDEX_MASK; // Word index

  // Find the next word used for a data word
  for (size_t j = w + 1; j < allocator->total_words; j++) {
    if (get_bit(allocator->word_type_bitmap, j) == 1) {
      return MSB_MASK | (j << 5); // Next word index
    }
  }
  return (uint)-1; // No next word found
}

// Free the cell or word at a given index
uint allocator_free(Allocator *allocator, uint index) {
  if (index & MSB_MASK) { // Word index
    size_t w = (index >> 5) & WORD_INDEX_MASK;
    if (w >= allocator->total_words ||
        !get_bit(allocator->word_type_bitmap, w)) {
      return 0; // Invalid or not allocated
    }
    set_bit(allocator->word_type_bitmap, w, 0);
    for (size_t k = 0; k < 32; k++) {
      set_bit(allocator->cell_bitmap, w * 32 + k, 0);
    }
    return 1;
  } else { // Cell index
    size_t w = (index >> 5) & WORD_INDEX_MASK;
    size_t c = index & CELL_INDEX_MASK;
    if (w >= allocator->total_words || c >= 32 ||
        get_bit(allocator->word_type_bitmap, w)) {
      return 0; // Invalid or word is not used for cells
    }
    size_t cell_bit = w * 32 + c;
    if (!get_bit(allocator->cell_bitmap, cell_bit)) {
      return 0; // Not allocated
    }
    set_bit(allocator->cell_bitmap, cell_bit, 0);
    return 1;
  }
}

// Get a cell or word at specified index
uint allocator_get(Allocator *allocator, uint index) {
  if (index & MSB_MASK) { // Word
    size_t w = (index >> 5) & WORD_INDEX_MASK;
    return allocator->pool[w]; // No bounds checking as per note
  } else {                     // Cell
    size_t w = (index >> 5) & WORD_INDEX_MASK;
    size_t c = index & CELL_INDEX_MASK;
    word_t word = allocator->pool[w];
    return (word >> (c * CELL_SIZE)) & 0x3; // Extract 2-bit cell value
  }
}

// Check if cell or word at provided index is set
uint allocator_is_set(Allocator *allocator, uint index) {
  if (index & MSB_MASK) { // Word
    size_t w = (index >> 5) & WORD_INDEX_MASK;
    if (w >= allocator->total_words)
      return (uint)-1;
    return get_bit(allocator->word_type_bitmap, w); // 1 if allocated, 0 if not
  } else {                                          // Cell
    size_t w = (index >> 5) & WORD_INDEX_MASK;
    size_t c = index & CELL_INDEX_MASK;
    if (w >= allocator->total_words || c >= 32)
      return (uint)-1;
    if (get_bit(allocator->word_type_bitmap, w))
      return 0; // Word used for a data word
    return get_bit(allocator->cell_bitmap,
                   w * 32 + c); // 1 if allocated, 0 if not
  }
}

// Initialize the allocator
void allocator_init(Allocator *allocator, size_t initial_words) {
  allocator->total_words = initial_words;
  allocator->pool = malloc(initial_words * sizeof(word_t));
  memset(allocator->pool, 0, initial_words * sizeof(word_t));

  allocator->word_type_bitmap_size = (initial_words + 63) / 64;
  allocator->word_type_bitmap =
      malloc(allocator->word_type_bitmap_size * sizeof(uint64_t));
  memset(allocator->word_type_bitmap, 0,
         allocator->word_type_bitmap_size * sizeof(uint64_t));

  allocator->cell_bitmap_size = (initial_words * 32 + 63) / 64;
  allocator->cell_bitmap =
      malloc(allocator->cell_bitmap_size * sizeof(uint64_t));
  memset(allocator->cell_bitmap, 0,
         allocator->cell_bitmap_size * sizeof(uint64_t));
}

// Destroy the allocator and free memory
void allocator_destroy(Allocator *allocator) {
  free(allocator->pool);
  free(allocator->word_type_bitmap);
  free(allocator->cell_bitmap);
  allocator->total_words = 0;
  allocator->word_type_bitmap_size = 0;
  allocator->cell_bitmap_size = 0;
}

// Set the value at the specified index (cell or word)
uint allocator_set(Allocator *allocator, uint index, uint value) {
  if (index & MSB_MASK) { // Word index
    size_t w = (index >> 5) & WORD_INDEX_MASK;
    if (w >= allocator->total_words ||
        !get_bit(allocator->word_type_bitmap, w)) {
      return 0; // Invalid or not allocated
    }
    allocator->pool[w] = value;
    return 1;
  } else { // Cell index
    size_t w = (index >> 5) & WORD_INDEX_MASK;
    size_t c = index & CELL_INDEX_MASK;
    if (w >= allocator->total_words || c >= 32 ||
        get_bit(allocator->word_type_bitmap, w)) {
      return 0; // Invalid or word is not used for cells
    }
    size_t cell_bit = w * 32 + c;
    if (!get_bit(allocator->cell_bitmap, cell_bit)) {
      return 0; // Not allocated
    }
    // Clear the 2 bits and set the new value
    word_t *word = &allocator->pool[w];
    *word = (*word & ~(0x3ULL << (c * CELL_SIZE))) |
            ((value & 0x3) << (c * CELL_SIZE));
    return 1;
  }
}
