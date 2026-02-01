#include "cells_impl.h"

i64 cells_init(struct cells_t* cells, u64 capacity) {
  cells->data = calloc(1, capacity);
  if (!cells->data) {
    return -1;
  }
  cells->occupied_bitmap = _bitmap_alloc(capacity);
  if (!cells->occupied_bitmap) {
    free(cells->data);
    return -1;
  }
  cells->capacity = capacity;
  return 0;
}

void cells_free(struct cells_t* cells) {
  free(cells->data);
  free(cells->occupied_bitmap);
}

i64 cells_get_node(struct cells_t* cells, u64 index, byte* out_value) {
  (void)cells;
  (void)index;
  (void)out_value;
  (void)_bitmap_get_bit;
  (void)_bitmap_set_bit;
  return -1;
}
