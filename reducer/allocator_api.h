#ifndef REDUCER_ALLOCATOR_API_H
#define REDUCER_ALLOCATOR_API_H

#include "domain_api.h"
#include "vendor/arena.h"

// alignof not available in C99, use max alignment for simple types
#define ALIGNMENT_MAX (sizeof(void*) > 8 ? sizeof(void*) : 8)

struct allocator_vtable_t {
  opaque_t (*alloc)(opaque_t ctx, size_t len, size_t align);
  bool (*resize)(opaque_t ctx, opaque_t ptr, size_t old_size, size_t new_size, size_t align);
  opaque_t (*remap)(opaque_t ctx, opaque_t ptr, size_t old_size, size_t new_size, size_t align);
  void (*free)(opaque_t ctx, opaque_t ptr, size_t size, size_t align);
};

struct allocator_t {
  opaque_t ctx;
  const struct allocator_vtable_t* vtable;
};

MUH_PUBLIC struct allocator_t allocator_libc(void);
MUH_PUBLIC struct allocator_t allocator_arena(Arena* arena);

struct da_byte_t {
  struct allocator_t allocator;
  unsigned char* items;
  size_t count;
  size_t capacity;
};

struct da_u32_t {
  struct allocator_t allocator;
  u32* items;
  size_t count;
  size_t capacity;
};

#endif // REDUCER_ALLOCATOR_API_H
