#ifndef __EVAL_COMMON__
#define __EVAL_COMMON__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef size_t uint;
#define ERROR_VALUE (uint)(-1)

#define debug(fmt, ...)                                                        \
  printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);
#define debug_s(s) printf("[%s:%d] " s "\n", __FILE__, __LINE__);

#define EVAL_GET_TAG(integer) ((uint8_t)((integer) & 0xF))
#define EVAL_GET_PAYLOAD(integer) ((integer) >> 4)
#define EVAL_SET_TAG(integer, tag)                                             \
  (((integer) & ~((uint64_t)0xF)) | ((uint64_t)(tag) & 0xF))
#define EVAL_SET_PAYLOAD(integer, payload)                                     \
  (((payload) << 4) | ((integer) & 0xF))

#define EVAL_NIL 0
#define EVAL_TREE 1
#define EVAL_APPLY 2
#define EVAL_NATIVE 3

#define EVAL_TAG_NUMBER 0
#define EVAL_TAG_INDEX 1
#define EVAL_TAG_FUNC 2

typedef struct EvalState_impl *EvalState;
typedef struct Allocator_impl *Allocator;
typedef uint64_t word_t;

uint eval_init(EvalState *state, const char *program);
uint eval_free(EvalState *state);
uint eval_step(EvalState state);
Allocator eval_get_memory(EvalState state);
uint8_t eval_get_error(EvalState state, const char **message);

uint eval_cells_init(Allocator *cells, size_t words_count);
uint eval_cells_free(Allocator *cells);
uint eval_cells_get(Allocator cells, size_t index);
uint eval_cells_get_word(Allocator cells, size_t index);
uint eval_cells_set(Allocator cells, size_t index, uint8_t value);
uint eval_cells_set_word(Allocator cells, size_t index, word_t value);
uint eval_cells_is_set(Allocator cells, size_t index);
uint eval_cells_capacity(Allocator cells);
uint eval_cells_clear(Allocator cells);

uint eval_encode_parse(Allocator cells, const char *program);
void eval_encode_dump(Allocator cells, size_t root);

#define BITS_PER_WORD (sizeof(word_t) * 8)
#define BITMAP_SIZE(cap) (((cap) + BITS_PER_WORD - 1) / BITS_PER_WORD)
uint _bitmap_get_bit(const word_t *bitmap, size_t index);
void _bitmap_set_bit(word_t *bitmap, size_t index, uint value);

#define BITS_PER_CELL 2
#define CELLS_PER_WORD (BITS_PER_WORD / BITS_PER_CELL)

#endif // __EVAL_COMMON__
