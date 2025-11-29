#ifndef __INTERNAL_MEMORY_MEMORY_IMPL_H__
#define __INTERNAL_MEMORY_MEMORY_IMPL_H__

#include "internal/domain/domain_api.h"
#include <stddef.h>

typedef struct {
  size_t key;
  size_t value;
} cell_word_t;

struct allocator_t {
  uint* cells;
  uint* cells_bitmap;
  size_t cells_capacity;

  cell_word_t* payload_index;

  sint* payloads;
};

#endif
