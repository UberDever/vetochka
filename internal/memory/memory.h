#ifndef __EVAL_MEMORY__
#define __EVAL_MEMORY__

#include "api.h"

#define BITS_PER_CELL    2
#define CELLS_PER_WORD   (BITS_PER_WORD / BITS_PER_CELL)
#define BITS_PER_WORD    (sizeof(uint) * 8)
#define BITMAP_SIZE(cap) (((cap) + BITS_PER_WORD - 1) / BITS_PER_WORD)

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

static inline u8 _tv_get_tag(uint tagged_value) {
  return (u8)(tagged_value & 0xF);
}

static inline sint _tv_get_payload_signed(uint tagged_value) {
  return (sint)(tagged_value & ~0xFULL) >> 4;
}

static inline uint _tv_get_payload_unsigned(uint tagged_value) {
  return tagged_value >> 4;
}

static inline uint _tv_set_tag(uint tagged_value, u8 new_tag) {
  return (tagged_value & ~0xFULL) | (new_tag & 0xF);
}

static inline uint _tv_set_payload_signed(uint tagged_value, sint new_payload) {
  uint tag_bits = tagged_value & 0xF;
  return ((uint)new_payload << 4) | tag_bits;
}

static inline uint _tv_set_payload_unsigned(uint tagged_value, uint new_payload) {
  uint tag_bits = tagged_value & 0xF;
  return (new_payload << 4) | tag_bits;
}

static inline uint _tv_new_tagged_value_signed(u8 tag, sint payload) {
  return ((uint)payload << 4) | (tag & 0xF);
}

static inline uint _tv_new_tagged_value_unsigned(u8 tag, uint payload) {
  return (payload << 4) | (tag & 0xF);
}

#endif
