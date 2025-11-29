#ifndef __INTERNAL_JSON_PARSER_JSON_PARSER_IMPL_H__
#define __INTERNAL_JSON_PARSER_JSON_PARSER_IMPL_H__

#include "internal/utility/string_buffer_api.h"
#include <stdbool.h>
#include <stddef.h>

struct jsmntok;

struct json_parser_t {
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
};

#endif
