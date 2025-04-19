#ifndef __EVAL_COMMON__
#define __EVAL_COMMON__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int32_t i32;
typedef int64_t i64;
typedef intptr_t sint;
typedef uintptr_t uint;

#define ERR_VAL -1

#define debug(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);
#define debug_s(s)      printf("[%s:%d] " s "\n", __FILE__, __LINE__);

#define EVAL_NIL  0
#define EVAL_TREE 1
// #define EVAL_APPLY 2
#define EVAL_REF 3

#define EVAL_TAG_NUMBER 0
#define EVAL_TAG_INDEX  1
#define EVAL_TAG_FUNC   2

typedef struct EvalState_impl* EvalState;
typedef struct Allocator_impl* Allocator;

typedef enum {
  StackEntryType_Invalid,
  StackEntryType_Index,
  StackEntryType_Calculated,
} StackEntryType;

typedef enum {
  CalculatedIndexType_Invalid = 0,
  CalculatedIndexType_Rule2 = 2,
  CalculatedIndexType_Rule3c = 3,
} CalculatedIndexType;

typedef struct {
  StackEntryType type;

  union {
    size_t as_index;

    struct {
      CalculatedIndexType type;
    } as_calculated_index;
  };
} StackEntry;

typedef StackEntry* Stack;

sint eval_init(EvalState* state, const char* program);
sint eval_free(EvalState* state);
void eval_step(EvalState state);
Allocator eval_get_memory(EvalState state);
Stack eval_get_stack(EvalState state);
u8 eval_get_error(EvalState state, const char** message);

sint eval_cells_init(Allocator* cells, size_t words_count);
sint eval_cells_free(Allocator* cells);
sint eval_cells_get(Allocator cells, size_t index);
sint eval_cells_get_word(Allocator cells, size_t index);
sint eval_cells_set(Allocator cells, size_t index, uint8_t value);
sint eval_cells_set_word(Allocator cells, size_t index, i64 value);
sint eval_cells_is_set(Allocator cells, size_t index);
size_t eval_cells_capacity(Allocator cells);
sint eval_cells_clear(Allocator cells);

sint eval_encode_parse(Allocator cells, const char* program);
void eval_encode_dump(Allocator cells, size_t root);

// internal stuff

#define BITS_PER_CELL    2
#define CELLS_PER_WORD   (BITS_PER_WORD / BITS_PER_CELL)
#define BITS_PER_WORD    (sizeof(u64) * 8)
#define BITMAP_SIZE(cap) (((cap) + BITS_PER_WORD - 1) / BITS_PER_WORD)
u8 _bitmap_get_bit(const u64* bitmap, size_t index);
void _bitmap_set_bit(u64* bitmap, size_t index, u8 value);

void _errbuf_clear();
void _errbuf_write(const char* format, ...);

#define GET_CELL(cells, index)                                                                     \
  sint index##_cell = eval_cells_get(cells, index);                                                \
  EXPECT(index##_cell != ERR_VAL, ERROR_INVALID_CELL, "");

#define GET_WORD(cells, index)                                                                     \
  sint index##_word = eval_cells_get_word(cells, index);                                           \
  EXPECT(index##_cell != ERR_VAL, ERROR_INVALID_WORD, "");

#define DEREF(cells, index)                                                                        \
  if (index##_cell == EVAL_REF) {                                                                  \
    GET_WORD(cells, index);                                                                        \
    index += (sint)index##_word;                                                                   \
    size_t new_index = index;                                                                      \
    GET_CELL(cells, new_index)                                                                     \
    EXPECT(new_index_cell != EVAL_REF, ERROR_REF_TO_REF, "");                                      \
  }

// NOTE: uncomment when needed
#if 0
static inline u8 eval_tv_get_tag(u64 tagged_value) {
  return (u8)(tagged_value & 0xF);
}

static inline i64 eval_tv_get_payload_signed(u64 tagged_value) {
  return (i64)(tagged_value & ~0xFULL) >> 4;
}

static inline u64 eval_tv_get_payload_unsigned(u64 tagged_value) {
  return tagged_value >> 4;
}

static inline u64 eval_tv_set_tag(u64 tagged_value, u8 new_tag) {
  return (tagged_value & ~0xFULL) | (new_tag & 0xF);
}

static inline u64 eval_tv_set_payload_signed(u64 tagged_value, i64 new_payload) {
  u64 tag_bits = tagged_value & 0xF;
  return ((u64)new_payload << 4) | tag_bits;
}

static inline u64 eval_tv_set_payload_unsigned(u64 tagged_value, u64 new_payload) {
  u64 tag_bits = tagged_value & 0xF;
  return (new_payload << 4) | tag_bits;
}

static inline u64 eval_tv_new_tagged_value_signed(u8 tag, i64 payload) {
  return ((u64)payload << 4) | (tag & 0xF);
}

static inline u64 eval_tv_new_tagged_value_unsigned(u8 tag, u64 payload) {
  return (payload << 4) | (tag & 0xF);
}
#endif

#endif // __EVAL_COMMON__
