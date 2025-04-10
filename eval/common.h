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

typedef struct Memory_impl* Memory;
uint eval_memory_init(Memory* arena, uint num_cells);
uint eval_memory_free(Memory* arena);
uint eval_memory_set_cell(Memory arena, uint index, uint value);
uint eval_memory_get_cell(Memory arena, uint index);
