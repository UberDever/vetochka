#ifndef __EVAL_ENCODE__
#define __EVAL_ENCODE__

#include <stdbool.h>

#include "internal/eval/eval_api.h"

struct string_buffer_t;
struct json_parser_t;
struct allocator_t;

void _eval_debug_dump(eval_state_t* state, struct string_buffer_t* buffer);
sint _eval_load_json(struct json_parser_t* parser, eval_state_t* state);
sint _eval_cells_load_json(struct json_parser_t* parser, eval_state_t* state);
sint _eval_cells_dump_json(struct string_buffer_t* json_out, struct allocator_t* cells);

#endif
