#include "common.h"
#include <stdlib.h>
#include <string.h>

typedef uint64_t word_t;

#define CELL_SIZE     2
#define CELLS_IN_WORD (sizeof(word_t) * 8 / CELL_SIZE)
#define CELL_MASK     ((1 << CELL_SIZE) - 1)

struct Memory_impl {
  word_t* data;
  size_t size;
};

// TODO: error handling
uint eval_memory_init(struct Memory_impl** arena, uint num_cells) {
  *arena = malloc(sizeof(struct Memory_impl));
  memset(*arena, 0, sizeof(struct Memory_impl));
  (*arena)->size = num_cells;
  size_t num_cell_ts = (num_cells + CELLS_IN_WORD - 1) / CELLS_IN_WORD;
  (*arena)->data = (word_t*)calloc(num_cell_ts, sizeof(word_t));
  return 0;
}

uint eval_memory_set_cell(struct Memory_impl* arena, uint index, uint value) {
  if (index >= arena->size || value > CELL_MASK) {
    return -1;
  }
  size_t cell_t_index = index / CELLS_IN_WORD;
  size_t bit_offset = (index % CELLS_IN_WORD) * CELL_SIZE;
  arena->data[cell_t_index] &= ~((word_t)CELL_MASK << bit_offset);
  arena->data[cell_t_index] |= ((word_t)value & CELL_MASK) << bit_offset;
  return 0;
}

uint eval_memory_get_cell(struct Memory_impl* arena, uint index) {
  if (index >= arena->size) {
    return -1;
  }
  size_t cell_t_index = index / CELLS_IN_WORD;
  size_t bit_offset = (index % CELLS_IN_WORD) * CELL_SIZE;
  return (arena->data[cell_t_index] >> bit_offset) & CELL_MASK;
}

uint eval_memory_free(struct Memory_impl** arena) {
  free((*arena)->data);
  (*arena)->data = NULL;
  (*arena)->size = 0;
  free(*arena);
  *arena = NULL;
  return 0;
}
