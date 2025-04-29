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

typedef struct EvalState_impl* EvalState;
typedef struct Allocator_impl* Allocator;
typedef struct StackEntry* Stack;
struct string_buffer_t;

sint eval_init(EvalState* state);
sint eval_free(EvalState* state);
void eval_step(EvalState state);
u8 eval_get_error(EvalState state, const char** message);
sint eval_dump_json(struct string_buffer_t* json_out, EvalState state);
sint eval_load_json(const char* json, EvalState state);

sint eval_cells_init(Allocator* cells, size_t words_count);
sint eval_cells_free(Allocator* cells);
sint eval_cells_get(Allocator cells, size_t index);
sint eval_cells_get_word(Allocator cells, size_t index);
sint eval_cells_set(Allocator cells, size_t index, uint8_t value);
sint eval_cells_set_word(Allocator cells, size_t index, i64 value);
sint eval_cells_is_set(Allocator cells, size_t index);
sint eval_cells_clear(Allocator cells);
