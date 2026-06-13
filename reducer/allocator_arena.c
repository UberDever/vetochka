#include "allocator_api.h"

#include "domain_api.h"
#include "vendor/arena.h"

#include <execinfo.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ALLOCATOR_STACKTRACE_DEPTH 64

// NOTE: currently for linux only
MUH_NORETURN MUH_COLD MUH_NOINLINE static void _allocator_arena_trap_unsupported(
    const char* op, opaque_t ctx, opaque_t ptr, size_t old_size, size_t new_size, size_t align) {
  fprintf(
      stderr,
      "\n"
      "allocator panic: arena allocator does not support %s\n"
      "\n"
      "operation:\n"
      "  op:        %s\n"
      "  arena ctx: %p\n"
      "  ptr:       %p\n"
      "  old_size:  %zu\n"
      "  new_size:  %zu\n"
      "  align:     %zu\n"
      "\n"
      "stacktrace:\n",
      op,
      op,
      (void*)ctx,
      (void*)ptr,
      old_size,
      new_size,
      align);

  void* frames[ALLOCATOR_STACKTRACE_DEPTH];
  int n = backtrace(frames, ALLOCATOR_STACKTRACE_DEPTH);
  backtrace_symbols_fd(frames, n, STDERR_FILENO);

  fprintf(stderr, "\n");

#if defined(__GNUC__) || defined(__clang__)
  __builtin_trap();
#else
  abort();
#endif
}

static opaque_t _allocator_arena_alloc(opaque_t ctx, size_t len, size_t align) {
  return arena_alloc_aligned(ctx, len, (unsigned int)align);
}

static bool _allocator_arena_resize(
    opaque_t ctx, opaque_t ptr, size_t old_size, size_t new_size, size_t align) {
  _allocator_arena_trap_unsupported("resize", ctx, ptr, old_size, new_size, align);
}

static opaque_t _allocator_arena_remap(
    opaque_t ctx, opaque_t ptr, size_t old_size, size_t new_size, size_t align) {
  _allocator_arena_trap_unsupported("remap", ctx, ptr, old_size, new_size, align);
}

static void _allocator_arena_free(opaque_t ctx, opaque_t ptr, size_t size, size_t align) {
  (void)ctx;
  (void)ptr;
  (void)size;
  (void)align;
  MUH_DEBUG("WARNING: This probably should not happen...");
}

static const struct allocator_vtable_t _allocator_arena_vtable = {
    .alloc = _allocator_arena_alloc,
    .resize = _allocator_arena_resize,
    .remap = _allocator_arena_remap,
    .free = _allocator_arena_free,
};

struct allocator_t allocator_arena(Arena* arena) {
  return (struct allocator_t){
      .ctx = arena,
      .vtable = &_allocator_arena_vtable,
  };
}
