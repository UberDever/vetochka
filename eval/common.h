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

#if 0
Node node_new_tree(sint lhs, sint rhs);
Node node_new_app(uint lhs, uint rhs);
Node node_new_data(uint data);
Node node_new_invalid();

uint node_tag(Node node);
sint node_lhs(Node node);
sint node_rhs(Node node);
uint node_data(Node node);

uint node_tag_tree();
uint node_tag_app();
uint node_tag_data();

typedef struct EvalState_impl *EvalState;

uint eval_init(EvalState *state, uint root, const uint *nodes, uint nodes_size);
uint eval_free(EvalState *state);

uint eval_step(EvalState state);
uint eval_eval(EvalState state);
uint eval_get_error(EvalState state, uint *code, char **error);
#endif

#define GET_TAG(integer) ((uint8_t)((integer) & 0xFF))
#define GET_PAYLOAD(integer) ((integer) >> 8)
#define SET_TAG(integer, tag)                                                  \
  (((integer) & ~((uint64_t)0xFF)) | ((uint64_t)(tag) & 0xFF))
#define SET_PAYLOAD(integer, payload) (((payload) << 8) | ((integer) & 0xFF))

#define EVAL_NIL 0
#define EVAL_TREE 1
#define EVAL_APPLY 2
#define EVAL_NATIVE 3

#define EVAL_TAG_NUMBER 0
#define EVAL_TAG_INDEX 1

typedef struct EvalState_impl *EvalState;
typedef struct Allocator_impl *Allocator;
typedef uint64_t word_t;

uint eval_init(EvalState *state, const char *program);
uint eval_free(EvalState *state);
uint eval_step(EvalState state, bool *matched);
Allocator eval_get_memory(EvalState state);

uint eval_cells_init(Allocator *cells, size_t words_count);
uint eval_cells_free(Allocator *cells);
uint eval_cells_get(Allocator cells, size_t index);
uint eval_cells_get_word(Allocator cells, size_t index);
uint eval_cells_set(Allocator cells, size_t index, uint8_t value);
uint eval_cells_set_word(Allocator cells, size_t index, word_t value);
uint eval_cells_is_set(Allocator cells, size_t index);
uint eval_cells_clear(Allocator cells);

uint eval_encode_parse(Allocator cells, const char *program);

#endif // __EVAL_COMMON__