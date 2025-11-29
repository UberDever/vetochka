#ifndef __INTERNAL_MEMORY_MEMORY_API_H__
#define __INTERNAL_MEMORY_MEMORY_API_H__

#include "internal/domain/domain_api.h"
#include <stddef.h>

#define BITS_PER_CELL    2
#define CELLS_PER_WORD   (BITS_PER_WORD / BITS_PER_CELL)
#define BITS_PER_WORD    (sizeof(uint) * 8)
#define BITMAP_SIZE(cap) (((cap) + BITS_PER_WORD - 1) / BITS_PER_WORD)

typedef struct allocator_t allocator_t;

sint eval_cells_init(allocator_t** alloc, size_t words_count);
sint eval_cells_free(allocator_t** alloc);
sint eval_cells_get(allocator_t* cells, size_t index);
sint eval_cells_get_word(allocator_t* cells, size_t index, sint* word);
sint eval_cells_set(allocator_t* cells, size_t index, uint8_t value);
sint eval_cells_set_word(allocator_t* cells, size_t index, sint value);
sint eval_cells_is_set(allocator_t* cells, size_t index);
sint eval_cells_reset(allocator_t* cells);

#endif
