#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef int8_t i8;
typedef size_t uint;
typedef ssize_t sint;
typedef uint Node;

#define debug(fmt, ...)                                                        \
  printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);
#define debug_s(s) printf("[%s:%d] " s "\n", __FILE__, __LINE__);

#define NODE_TREE 0
#define NODE_APP 1
#define NODE_DATA 2

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

typedef struct EvalState_s* EvalState;

uint eval_init(EvalState* state, uint root, const uint *nodes,
          uint nodes_size);
uint eval_free(EvalState* state);

uint eval_step(EvalState state);
uint eval_eval(EvalState state);
uint eval_get_error(EvalState state, uint* code, char** error);

typedef uint64_t word_t;
typedef struct {
  word_t *pool;               // Array of 64-bit words
  size_t total_words;         // Number of 64-bit words in the pool
  uint64_t *word_type_bitmap; // Bitmap: 0 = word used for cells, 1 = used for a
                              // data word
  size_t word_type_bitmap_size; // Number of uint64_t in word_type_bitmap
  uint64_t *cell_bitmap; // Bitmap: allocation status of each cell (total_words
                         // * 32 bits)
  size_t cell_bitmap_size; // Number of uint64_t in cell_bitmap
} Allocator;


void allocator_init(Allocator *allocator, size_t initial_words);
void allocator_destroy(Allocator *allocator);
uint allocator_allocate(Allocator *allocator, uint type);
uint allocator_next_cell(Allocator *allocator, uint index);
uint allocator_next_word(Allocator *allocator, uint index);
uint allocator_free(Allocator *allocator, uint index);
uint allocator_get(Allocator *allocator, uint index);
uint allocator_set(Allocator *allocator, uint index, uint value);
uint allocator_is_set(Allocator *allocator, uint index);
