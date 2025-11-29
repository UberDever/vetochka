#ifndef __INTERNAL_MEMORY_BITMAP_API_H__
#define __INTERNAL_MEMORY_BITMAP_API_H__

#include "internal/domain/domain_api.h"
#include <stddef.h>

u8 _bitmap_get_bit(const uint* bitmap, size_t index);
void _bitmap_set_bit(uint* bitmap, size_t index, u8 value);

#endif
