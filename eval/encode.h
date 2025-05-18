#ifndef __EVAL_ENCODE__
#define __EVAL_ENCODE__

#include <stdbool.h>

#include "api.h"
#include "util.h"

enum json_token_t {
  JSON_TOKEN_NULL,
  JSON_TOKEN_BOOL,
  JSON_TOKEN_NUMBER,
  JSON_TOKEN_STRING,
  JSON_TOKEN_ARRAY,
  JSON_TOKEN_OBJECT,
};

struct jsmntok;

typedef struct json_parser_t {
  const char* json;
  struct jsmntok* tokens;
  size_t tokens_len;
  size_t cur_token;
  bool was_err;
  bool at_eof;

  size_t entries_count;

  enum {
    JSON_DIGESTED_INVALID,
    JSON_DIGESTED_NULL,
    JSON_DIGESTED_BOOL,
    JSON_DIGESTED_NUMBER,
    JSON_DIGESTED_STRING,
    JSON_DIGESTED_ARRAY,
    JSON_DIGESTED_OBJECT,
  } digested;

  string_buffer_t digested_string;

  union {
    double digested_number;
    bool digested_bool;
  };
} json_parser_t;

// NOTE: doesn't take ownership of json
sint _json_parser_init(const char* json, json_parser_t* parser);
void _json_parser_free(json_parser_t* parser);
bool _json_parser_match(json_parser_t* parser, enum json_token_t token);
bool _json_parser_eat(json_parser_t* parser, enum json_token_t token);
const char* _json_parser_get_string(json_parser_t* parser);

#define _JSON_PARSER_EAT(type, errval)                                                             \
  if (parser->at_eof) {                                                                            \
    goto cleanup;                                                                                  \
  }                                                                                                \
  if (!_json_parser_eat(parser, JSON_TOKEN_##type)) {                                              \
    logg_s("failed to eat " #type);                                                                \
    err = errval;                                                                                  \
    goto cleanup;                                                                                  \
  }                                                                                                \
  if (parser->was_err) {                                                                           \
    err = errval;                                                                                  \
    goto cleanup;                                                                                  \
  }

#define _JSON_PARSER_EAT_KEY(key, errval)                                                          \
  _JSON_PARSER_EAT(STRING, 1);                                                                     \
  if (strcmp(_json_parser_get_string(parser), key) != 0) {                                         \
    err = errval;                                                                                  \
    logg_s("expected " key " field");                                                              \
    goto cleanup;                                                                                  \
  }

void _eval_debug_dump(eval_state_t* state, string_buffer_t* buffer);

sint _eval_load_json(struct json_parser_t* parser, eval_state_t* state);
sint _eval_cells_load_json(struct json_parser_t* parser, eval_state_t* state);

sint _eval_cells_dump_json(string_buffer_t* json_out, allocator_t* cells);

#endif
