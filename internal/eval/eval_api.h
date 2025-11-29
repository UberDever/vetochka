#ifndef __INTERNAL_EVAL_EVAL_API_H__
#define __INTERNAL_EVAL_EVAL_API_H__

#include "internal/domain/domain_api.h"
#include <stddef.h>

#define SIGIL_NIL  0
#define SIGIL_TREE 1
#define SIGIL_REF  2

#define TOKEN_APPLY SIZE_MAX

struct allocator_t;
struct string_buffer_t;
typedef struct eval_state_t eval_state_t;

sint eval_init(eval_state_t** state);
sint eval_free(eval_state_t** state);
sint eval_step(eval_state_t* state);
sint eval_dump_json(struct string_buffer_t* json_out, eval_state_t* state);
sint eval_load_json(const char* json, eval_state_t* state);
sint eval_reset(eval_state_t* state);
sint eval_add_native(eval_state_t* state, const char* name, uint symbol);
void eval_reset_errors(eval_state_t* state);
sint _eval_reset_cells(eval_state_t* state);

sint eval_get_native(eval_state_t* state, const char* name, uint* symbol);
struct allocator_t* eval_get_cells(eval_state_t* state);
size_t* eval_get_apply_stack(eval_state_t* state);
size_t* eval_get_result_stack(eval_state_t* state);
uint* eval_get_free_bitmap(eval_state_t* state);

size_t _eval_get_left_node(eval_state_t* state, size_t root_index);
size_t _eval_get_right_node(eval_state_t* state, size_t root_index);

#endif
