#ifndef __INTERNAL_JSON_PARSER_JSON_PARSER_API_H__
#define __INTERNAL_JSON_PARSER_JSON_PARSER_API_H__

#include "internal/domain/domain_api.h"
#include "internal/utility/string_buffer_api.h"
#include <stdbool.h>
#include <stddef.h>

#define _JSON_PARSER_EAT(type, errval)                                                             \
  if (parser->at_eof) {                                                                            \
    goto error;                                                                                    \
  }                                                                                                \
  if (!_json_parser_eat(parser, JSON_TOKEN_##type)) {                                              \
    logg_s("failed to eat " #type);                                                                \
    err = errval;                                                                                  \
    goto error;                                                                                    \
  }                                                                                                \
  if (parser->was_err) {                                                                           \
    err = errval;                                                                                  \
    goto error;                                                                                    \
  }

#define _JSON_PARSER_EAT_KEY(key, errval)                                                          \
  _JSON_PARSER_EAT(STRING, 1);                                                                     \
  if (strcmp(_json_parser_get_string(parser), key) != 0) {                                         \
    err = errval;                                                                                  \
    logg_s("expected " key " field");                                                              \
    goto error;                                                                                    \
  }

enum json_token_t {
  JSON_TOKEN_NULL,
  JSON_TOKEN_BOOL,
  JSON_TOKEN_INTEGER,
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
    JSON_DIGESTED_INTEGER,
    JSON_DIGESTED_STRING,
    JSON_DIGESTED_ARRAY,
    JSON_DIGESTED_OBJECT,
  } digested;

  struct string_buffer_t digested_string;

  union {
    double digested_integer;
    bool digested_bool;
  };
} json_parser_t;

// NOTE: doesn't take ownership of json
sint _json_parser_init(const char* json, json_parser_t* parser);
void _json_parser_free(json_parser_t* parser);
bool _json_parser_match(json_parser_t* parser, enum json_token_t token);
bool _json_parser_eat(json_parser_t* parser, enum json_token_t token);
const char* _json_parser_get_string(json_parser_t* parser);

#endif
