#include "json_parser_api.h"

#define JSMN_HEADER
#include "third_party/jsmn/jsmn.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// NOTE: doesn't take ownership of json
sint _json_parser_init(const char* json, json_parser_t* parser) {
  *parser = (json_parser_t){};
  parser->json = json;

  jsmn_parser p = {};
  jsmn_init(&p);
  size_t tokens_len = 32;
  jsmntok_t* tokens = malloc(tokens_len * sizeof(jsmntok_t));
  while (true) {
    assert(tokens_len <= 2 << 20); // NOTE: pretty random
    int res = jsmn_parse(&p, json, strlen(json), tokens, tokens_len);
    if (res == JSMN_ERROR_NOMEM) {
      tokens_len *= 2;
      tokens = realloc(tokens, tokens_len * sizeof(jsmntok_t));
      continue;
    }
    if (res < 0) {
      return res;
    }
    tokens_len = res;
    break;
  }
  parser->tokens = tokens;
  parser->tokens_len = tokens_len;
  _sb_init(&parser->digested_string);
  return 0;
}

void _json_parser_free(json_parser_t* parser) {
  _sb_free(&parser->digested_string);
  free(parser->tokens);
}

bool _json_parser_match(json_parser_t* parser, enum json_token_t token) {
  if (parser->was_err || parser->at_eof) {
    return false;
  }
  jsmntok_t t = parser->tokens[parser->cur_token];
  switch (token) {
    case JSON_TOKEN_NULL: {
      return t.type == JSMN_PRIMITIVE && parser->json[t.start] == 'n';
    }
    case JSON_TOKEN_BOOL: {
      return t.type == JSMN_PRIMITIVE
             && (parser->json[t.start] == 'f' || parser->json[t.start] == 't');
    }
    case JSON_TOKEN_INTEGER: {
      return t.type == JSMN_PRIMITIVE
             && (isdigit(parser->json[t.start]) || parser->json[t.start] == '-');
    }
    case JSON_TOKEN_STRING: {
      return t.type == JSMN_STRING;
    }
    case JSON_TOKEN_ARRAY: {
      return t.type == JSMN_ARRAY;
    }
    case JSON_TOKEN_OBJECT: {
      return t.type == JSMN_OBJECT;
    }
    default: parser->was_err = true; return false;
  }
}

static void json_digest(json_parser_t* parser) {
  parser->digested = JSON_DIGESTED_INVALID;
  _sb_clear(&parser->digested_string);
  parser->digested_integer = 0;
  parser->entries_count = 0;

  jsmntok_t t = parser->tokens[parser->cur_token];

  switch (t.type) {
    case JSMN_UNDEFINED: {
      parser->was_err = true;
      goto error;
    }
    case JSMN_OBJECT: {
      parser->digested = JSON_DIGESTED_OBJECT;
      parser->entries_count = t.size;
      goto error;
    }
    case JSMN_ARRAY: {
      parser->digested = JSON_DIGESTED_ARRAY;
      parser->entries_count = t.size;
      goto error;
    }
    case JSMN_STRING: {
      parser->digested = JSON_DIGESTED_STRING;
      _sb_append_data(&parser->digested_string, &parser->json[t.start], t.end - t.start);
      goto error;
    }
    case JSMN_PRIMITIVE: {
      if (_json_parser_match(parser, JSON_TOKEN_NULL)) {
        parser->digested = JSON_DIGESTED_NULL;
        goto error;
      }
      if (_json_parser_match(parser, JSON_TOKEN_BOOL)) {
        parser->digested = JSON_TOKEN_BOOL;
        parser->digested_bool = parser->json[t.start] == 't';
        goto error;
      }
      if (_json_parser_match(parser, JSON_TOKEN_INTEGER)) {
        parser->digested = JSON_TOKEN_INTEGER;
        _sb_append_data(&parser->digested_string, &parser->json[t.start], t.end - t.start);
        const char* tok_str = _sb_str_view(&parser->digested_string);
        char* endptr;
        errno = 0;
        parser->digested_integer = strtod(tok_str, &endptr);
        _sb_clear(&parser->digested_string);
        if (errno == ERANGE || endptr == tok_str || *endptr != '\0') {
          parser->was_err = true;
          goto error;
        }
      }
    }
  }

error:
  parser->cur_token++;
  if (parser->cur_token >= parser->tokens_len) {
    parser->at_eof = true;
  }
}

bool _json_parser_eat(json_parser_t* parser, enum json_token_t token) {
  if (parser->was_err || parser->at_eof) {
    return false;
  }
  bool res = _json_parser_match(parser, token);
  if (res == false || parser->was_err || parser->at_eof) {
    return false;
  }
  json_digest(parser);
  return true;
}

const char* _json_parser_get_string(json_parser_t* parser) {
  if (parser->was_err || parser->at_eof || parser->digested != JSON_DIGESTED_STRING) {
    return NULL;
  }
  return _sb_str_view(&parser->digested_string);
}
