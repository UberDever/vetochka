/* Dynamic array facilities extracted from nob.h v3.8.2
   Public Domain - https://github.com/tsoding/nob.h

   Original author: Alexey "Tsoding" Kutepov.
   This file keeps only dynamic-array macros, with nob_ prefixes removed.
*/

#ifndef DA_H_
#define DA_H_

#include <assert.h>
#include <stddef.h>
#include <string.h>

#ifndef DA_ASSERT
#define DA_ASSERT assert
#endif

// Initial capacity of a dynamic array.
#ifndef DA_INIT_CAP
#define DA_INIT_CAP 256
#endif

#ifdef __cplusplus
#define DA_DECLTYPE_CAST(T) (decltype(T))
#else
#define DA_DECLTYPE_CAST(T)
#endif

#define da_reserve(da, expected_capacity)                                                          \
  do {                                                                                             \
    if ((expected_capacity) > (da)->capacity) {                                                    \
      if ((da)->capacity == 0) { (da)->capacity = DA_INIT_CAP; }                                   \
      while ((expected_capacity) > (da)->capacity) {                                               \
        (da)->capacity *= 2;                                                                       \
      }                                                                                            \
      (da)->items = DA_DECLTYPE_CAST((da)->items)(da)->allocator.vtable->remap(                    \
          (da)->allocator.ctx,                                                                     \
          (da)->items,                                                                             \
          (da)->count * sizeof(*(da)->items),                                                      \
          (da)->capacity * sizeof(*(da)->items),                                                   \
          ALIGNMENT_MAX);                                                                          \
      DA_ASSERT((da)->items != NULL && "Buy more RAM lol");                                        \
    }                                                                                              \
  } while (0)

// Append an item to a dynamic array.
#define da_append(da, item)                                                                        \
  do {                                                                                             \
    da_reserve((da), (da)->count + 1);                                                             \
    (da)->items[(da)->count++] = (item);                                                           \
  } while (0)

#define da_free(da)                                                                                \
  do {                                                                                             \
    if ((da)->items) {                                                                             \
      (da)->allocator.vtable->free(                                                                \
          (da)->allocator.ctx, (da)->items, (da)->capacity * sizeof(*(da)->items), ALIGNMENT_MAX); \
    }                                                                                              \
    (da)->items = NULL;                                                                            \
    (da)->count = 0;                                                                               \
    (da)->capacity = 0;                                                                            \
  } while (0)

// Append several items to a dynamic array.
#define da_append_many(da, new_items, new_items_count)                                             \
  do {                                                                                             \
    da_reserve((da), (da)->count + (new_items_count));                                             \
    memcpy((da)->items + (da)->count, (new_items), (new_items_count) * sizeof(*(da)->items));      \
    (da)->count += (new_items_count);                                                              \
  } while (0)

#define da_resize(da, new_size)                                                                    \
  do {                                                                                             \
    da_reserve((da), new_size);                                                                    \
    (da)->count = (new_size);                                                                      \
  } while (0)

#define da_pop(da)   (da)->items[(DA_ASSERT((da)->count > 0), --(da)->count)]
#define da_first(da) (da)->items[(DA_ASSERT((da)->count > 0), 0)]
#define da_last(da)  (da)->items[(DA_ASSERT((da)->count > 0), (da)->count - 1)]

#define da_remove_unordered(da, i)                                                                 \
  do {                                                                                             \
    size_t j = (i);                                                                                \
    DA_ASSERT(j < (da)->count);                                                                    \
    (da)->items[j] = (da)->items[--(da)->count];                                                   \
  } while (0)

// Foreach over Dynamic Arrays. Example:
// #include "somewhere/memory/api.h"
//
// typedef struct {
//     struct allocator_t allocator;
//     int *items;
//     size_t count;
//     size_t capacity;
// } Numbers;
//
// Numbers xs = { .allocator = allocator_libc() };
// da_append(&xs, 69);
// da_append(&xs, 420);
//
// da_foreach(int, x, &xs) {
//     size_t index = x - xs.items;
//     printf("%zu: %d\n", index, *x);
// }
#define da_foreach(Type, it, da) for (Type* it = (da)->items; it < (da)->items + (da)->count; ++it)

#endif // DA_H_
