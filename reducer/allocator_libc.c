#include "allocator_api.h"

#include <stdlib.h>

static opaque_t _allocator_libc_alloc(opaque_t ctx, size_t len, size_t align) {
  (void)ctx;
  (void)align;
  return malloc(len);
}

static bool _allocator_libc_resize(
    opaque_t ctx, opaque_t ptr, size_t old_size, size_t new_size, size_t align) {
  (void)ctx;
  (void)old_size;
  (void)align;
  void* result = realloc(ptr, new_size);
  if (result == NULL) { return false; }
  return true;
}

static opaque_t _allocator_libc_remap(
    opaque_t ctx, opaque_t ptr, size_t old_size, size_t new_size, size_t align) {
  (void)ctx;
  (void)old_size;
  (void)align;
  return realloc(ptr, new_size);
}

static void _allocator_libc_free(opaque_t ctx, opaque_t ptr, size_t size, size_t align) {
  (void)ctx;
  (void)size;
  (void)align;
  free(ptr);
}

static const struct allocator_vtable_t _allocator_libc_vtable = {
    .alloc = _allocator_libc_alloc,
    .resize = _allocator_libc_resize,
    .remap = _allocator_libc_remap,
    .free = _allocator_libc_free,
};

struct allocator_t allocator_libc(void) {
  return (struct allocator_t){
      .ctx = NULL,
      .vtable = &_allocator_libc_vtable,
  };
}
