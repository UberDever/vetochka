#ifndef __EVAL_MEMORY__
#define __EVAL_MEMORY__

#include "api.h"

#define BITS_PER_CELL    2
#define CELLS_PER_WORD   (BITS_PER_WORD / BITS_PER_CELL)
#define BITS_PER_WORD    (sizeof(u64) * 8)
#define BITMAP_SIZE(cap) (((cap) + BITS_PER_WORD - 1) / BITS_PER_WORD)

typedef struct {
  size_t key;
  size_t value;
} cell_word_t;

struct allocator_t {
  u64* cells;
  u64* cells_bitmap;
  size_t cells_capacity;

  cell_word_t* payload_index;

  i64* payloads;
};

static inline u8 _tv_get_tag(u64 tagged_value) {
  return (u8)(tagged_value & 0xF);
}

static inline i64 _tv_get_payload_signed(u64 tagged_value) {
  return (i64)(tagged_value & ~0xFULL) >> 4;
}

static inline u64 _tv_get_payload_unsigned(u64 tagged_value) {
  return tagged_value >> 4;
}

static inline u64 _tv_set_tag(u64 tagged_value, u8 new_tag) {
  return (tagged_value & ~0xFULL) | (new_tag & 0xF);
}

static inline u64 _tv_set_payload_signed(u64 tagged_value, i64 new_payload) {
  u64 tag_bits = tagged_value & 0xF;
  return ((u64)new_payload << 4) | tag_bits;
}

static inline u64 _tv_set_payload_unsigned(u64 tagged_value, u64 new_payload) {
  u64 tag_bits = tagged_value & 0xF;
  return (new_payload << 4) | tag_bits;
}

static inline u64 _tv_new_tagged_value_signed(u8 tag, i64 payload) {
  return ((u64)payload << 4) | (tag & 0xF);
}

static inline u64 _tv_new_tagged_value_unsigned(u8 tag, u64 payload) {
  return (payload << 4) | (tag & 0xF);
}

#endif
