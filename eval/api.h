#ifndef __EVAL_API__
#define __EVAL_API__

#include <stdint.h>
#include <sys/cdefs.h>
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

_Static_assert(sizeof(void (*)()) <= 8, "Function pointer too large");

typedef struct eval_state_t eval_state_t;
typedef struct allocator_t allocator_t;
typedef size_t (*native_symbol_t)(eval_state_t*, size_t);
typedef struct string_buffer_t string_buffer_t;

sint eval_init(eval_state_t** state);
sint eval_free(eval_state_t** state);
sint eval_step(eval_state_t* state);
u8 eval_get_error(eval_state_t* state, const char** message);
sint eval_dump_json(struct string_buffer_t* json_out, eval_state_t* state);
sint eval_load_json(const char* json, eval_state_t* state);
sint eval_reset(eval_state_t* state);
sint eval_add_native(eval_state_t* state, const char* name, native_symbol_t symbol);
sint eval_get_native(eval_state_t* state, const char* name, native_symbol_t* symbol);

sint eval_cells_init(allocator_t** cells, size_t words_count);
sint eval_cells_free(allocator_t** cells);
sint eval_cells_get(allocator_t* cells, size_t index);
sint eval_cells_get_word(allocator_t* cells, size_t index);
sint eval_cells_set(allocator_t* cells, size_t index, uint8_t value);
sint eval_cells_set_word(allocator_t* cells, size_t index, i64 value);
sint eval_cells_is_set(allocator_t* cells, size_t index);
sint eval_cells_reset(allocator_t* cells);

#endif
