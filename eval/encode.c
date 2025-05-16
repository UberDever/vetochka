#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "vendor/stb_ds.h"
#define JSMN_HEADER
#include "vendor/jsmn.h"

#include "common.h"

static char CELL_TO_CHAR[] = {'*', '^', '#'};

void _eval_debug_dump(EvalState state, string_buffer_t* buffer) {
  const size_t WINDOW_SIZE = 4;
  const size_t LINE_LEN = 120;
  const size_t WINDOWS_LINE = LINE_LEN / WINDOW_SIZE;

#define DUMP_BUFFER                                                                                \
  _sb_append_str(buffer, _sb_str_view(&indices_line));                                             \
  _sb_append_char(buffer, '\n');                                                                   \
  _sb_append_str(buffer, _sb_str_view(&cells_line));                                               \
  _sb_append_char(buffer, '\n');                                                                   \
  _sb_append_str(buffer, _sb_str_view(&words_line));                                               \
  _sb_append_char(buffer, '\n');                                                                   \
  _sb_printf(buffer, "apply: ");                                                                   \
  for (size_t i = 0; i < stbds_arrlenu(state->apply_stack); ++i) {                                 \
    size_t value = state->apply_stack[i];                                                          \
    if (value == TOKEN_APPLY) {                                                                    \
      _sb_printf(buffer, "%d ", -1);                                                               \
    } else {                                                                                       \
      _sb_printf(buffer, "%zu ", value);                                                           \
    }                                                                                              \
  }                                                                                                \
  _sb_append_char(buffer, '\n');                                                                   \
  _sb_printf(buffer, "result: ");                                                                  \
  for (size_t i = 0; i < stbds_arrlenu(state->result_stack); ++i) {                                \
    size_t value = state->result_stack[i];                                                         \
    _sb_printf(buffer, "%zu ", value);                                                             \
  }                                                                                                \
  _sb_append_char(buffer, '\n');                                                                   \
  _sb_append_str(buffer, "---------\n");

  string_buffer_t indices_line;
  _sb_init(&indices_line);
  string_buffer_t cells_line;
  _sb_init(&cells_line);
  string_buffer_t words_line;
  _sb_init(&words_line);

  char num_format[20];
  snprintf(num_format, sizeof(num_format), "%%-%zud", WINDOW_SIZE);
  char char_format[20];
  snprintf(char_format, sizeof(char_format), "%%-%zuc", WINDOW_SIZE);

  size_t i = 0;
  while (eval_cells_is_set(state->cells, i)) {
    if ((i % WINDOWS_LINE == 0) && i > 0) {
      DUMP_BUFFER
      _sb_clear(&indices_line);
      _sb_clear(&cells_line);
      _sb_clear(&words_line);
    }
    _sb_printf(&indices_line, num_format, i);
    u8 cell = eval_cells_get(state->cells, i);
    _sb_printf(&cells_line, char_format, CELL_TO_CHAR[cell]);
    // TODO: fix for natives
    if (cell == SIGIL_REF) {
      i64 ref = eval_cells_get_word(state->cells, i);
      if (ref != ERR_VAL) {
        u64 ref_index = ref + i;
        _sb_printf(&words_line, num_format, ref_index);
        i++;
        continue;
      }
    }

    for (size_t j = 0; j < WINDOW_SIZE; ++j) {
      _sb_append_char(&words_line, ' ');
    }
    i++;
  }

  DUMP_BUFFER

#undef DUMP_BUFFER

  _sb_free(&indices_line);
  _sb_free(&cells_line);
  _sb_free(&words_line);
}

// ********************** JSON COMMON **********************

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
    case JSON_TOKEN_NUMBER: {
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
  parser->digested_number = 0;
  parser->entries_count = 0;

  jsmntok_t t = parser->tokens[parser->cur_token];

  switch (t.type) {
    case JSMN_UNDEFINED: {
      parser->was_err = true;
      goto cleanup;
    }
    case JSMN_OBJECT: {
      parser->digested = JSON_DIGESTED_OBJECT;
      parser->entries_count = t.size;
      goto cleanup;
    }
    case JSMN_ARRAY: {
      parser->digested = JSON_DIGESTED_ARRAY;
      parser->entries_count = t.size;
      goto cleanup;
    }
    case JSMN_STRING: {
      parser->digested = JSON_DIGESTED_STRING;
      _sb_append_data(&parser->digested_string, &parser->json[t.start], t.end - t.start);
      goto cleanup;
    }
    case JSMN_PRIMITIVE: {
      if (_json_parser_match(parser, JSON_TOKEN_NULL)) {
        parser->digested = JSON_DIGESTED_NULL;
        goto cleanup;
      }
      if (_json_parser_match(parser, JSON_TOKEN_BOOL)) {
        parser->digested = JSON_TOKEN_BOOL;
        parser->digested_bool = parser->json[t.start] == 't';
        goto cleanup;
      }
      if (_json_parser_match(parser, JSON_TOKEN_NUMBER)) {
        parser->digested = JSON_TOKEN_NUMBER;
        _sb_append_data(&parser->digested_string, &parser->json[t.start], t.end - t.start);
        const char* tok_str = _sb_str_view(&parser->digested_string);
        char* endptr;
        errno = 0;
        parser->digested_number = strtod(tok_str, &endptr);
        _sb_clear(&parser->digested_string);
        if (errno == ERANGE || endptr == tok_str || *endptr != '\0') {
          parser->was_err = true;
          goto cleanup;
        }
      }
    }
  }

cleanup:
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

// ********************** JSON LOADING **********************

sint eval_load_json(const char* json, EvalState state) {
  sint err = 0;

  json_parser_t parser = {};
  err = _json_parser_init(json, &parser);
  if (err) {
    logg_s("failed to parse json");
    goto cleanup;
  }

  err = _eval_load_json(&parser, state);
  if (err) {
    logg_s("failed to parse json");
    goto cleanup;
  }

cleanup:
  return 0;
}

sint _eval_load_json(json_parser_t* parser, EvalState state) {
  sint err = 0;

  state->error_code = 0;

  _JSON_PARSER_EAT(OBJECT, 1);
  _JSON_PARSER_EAT_KEY("cells", 1)
  if (_json_parser_match(parser, JSON_TOKEN_NULL)) {
    _JSON_PARSER_EAT(NULL, 1);
  } else {
    err = _eval_reset_cells(state);
    if (err) {
      goto cleanup;
    }
    err = _eval_cells_load_json(parser, state);
    if (err) {
      goto cleanup;
    }

    size_t i = 0;
    while (eval_cells_is_set(state->cells, i)) {
      _bitmap_set_bit(state->free_bitmap, i, 1);
      i++;
    }
  }

  _JSON_PARSER_EAT_KEY("apply_stack", 1)
  if (_json_parser_match(parser, JSON_TOKEN_NULL)) {
    _JSON_PARSER_EAT(NULL, 1);
  } else {
    stbds_arrsetlen(state->apply_stack, 0);
    _JSON_PARSER_EAT(ARRAY, 1);
    size_t apply_count = parser->entries_count;
    for (size_t i = 0; i < apply_count; ++i) {
      _JSON_PARSER_EAT(NUMBER, 1);
      if (parser->digested_number == -1) {
        stbds_arrpush(state->apply_stack, TOKEN_APPLY);
      } else {
        stbds_arrpush(state->apply_stack, parser->digested_number);
      }
    }
  }

  _JSON_PARSER_EAT_KEY("result_stack", 1)
  if (_json_parser_match(parser, JSON_TOKEN_NULL)) {
    _JSON_PARSER_EAT(NULL, 1);
  } else {
    stbds_arrsetlen(state->result_stack, 0);
    _JSON_PARSER_EAT(ARRAY, 1);
    size_t apply_count = parser->entries_count;
    for (size_t i = 0; i < apply_count; ++i) {
      _JSON_PARSER_EAT(NUMBER, 1);
      stbds_arrpush(state->result_stack, parser->digested_number);
    }
  }

cleanup:
  return err;
}

static u8 get_cell(char symbol) {
  if (symbol == '*') {
    return SIGIL_NIL;
  }
  if (symbol == '^') {
    return SIGIL_TREE;
  }
  if (symbol == '#') {
    return SIGIL_REF;
  }
  assert(false && "unreachable");
}

sint _eval_cells_load_json(struct json_parser_t* parser, EvalState state) {
  sint err = 0;
  Allocator cells = state->cells;
  _JSON_PARSER_EAT(OBJECT, 1);
  _JSON_PARSER_EAT_KEY("state", 1)
  _JSON_PARSER_EAT(STRING, 1);
  const char* state_str = _json_parser_get_string(parser);
  for (size_t i = 0; i < strlen(state_str); ++i) {
    eval_cells_set(cells, i, get_cell(state_str[i]));
  }

  _JSON_PARSER_EAT_KEY("words", 1)
  _JSON_PARSER_EAT(ARRAY, 1);
  size_t words_count = parser->entries_count;
  for (size_t i = 0; i < words_count; ++i) {
    _JSON_PARSER_EAT(OBJECT, 1);
    _JSON_PARSER_EAT(STRING, 1);
    if (strcmp(_json_parser_get_string(parser), "ref") == 0) {
      _JSON_PARSER_EAT(NUMBER, 1);
      i64 ref_value = parser->digested_number;
      _JSON_PARSER_EAT_KEY("index", 1)
      _JSON_PARSER_EAT(NUMBER, 1);
      size_t ref_index = parser->digested_number;
      err = eval_cells_set_word(cells, ref_index, ref_value);
      if (err) {
        goto cleanup;
      }
      continue;
    }

    if (strcmp(_json_parser_get_string(parser), "index") == 0) {
      _JSON_PARSER_EAT(NUMBER, 1);
      size_t val_index = parser->digested_number;
      _JSON_PARSER_EAT_KEY("tag", 1)
      _JSON_PARSER_EAT(STRING, 1);
      const char* tag_str = _json_parser_get_string(parser);
      if (strcmp(tag_str, "function") == 0) {
        _JSON_PARSER_EAT_KEY("payload", 1)
        _JSON_PARSER_EAT(STRING, 1);
        const char* payload_str = _json_parser_get_string(parser);
        native_symbol_t symbol = NULL;
        err = eval_get_native(state, payload_str, &symbol);
        if (err) {
          logg("can't find native %s", payload_str);
          goto cleanup;
        }
        err = eval_cells_set_word(
            cells, val_index, eval_tv_new_tagged_value_signed(NATIVE_TAG_FUNC, (i64)symbol));
        if (err) {
          goto cleanup;
        }

        continue;
      }
    }
    assert(0 && "unreachable");
  }

cleanup:
  return err;
}

// ********************** JSON DUMPING **********************

#define CHECK(cond)                                                                                \
  if (!(cond)) {                                                                                   \
    goto cleanup;                                                                                  \
  }

static sint dump_apply_stack(struct string_buffer_t* json_out, const size_t* stack) {
  sint result = 0;
  _sb_printf(json_out, "\"apply_stack\": [");
  for (size_t i = 0; i < stbds_arrlenu(stack); ++i) {
    size_t e = stack[i];
    if (e == TOKEN_APPLY) {
      _sb_printf(json_out, "%d, ", -1);
    } else {
      _sb_printf(json_out, "%zu, ", e);
    }
  }
  _sb_try_chop_suffix(json_out, ", ");
  _sb_append_str(json_out, "]");

  return result;
}

static sint dump_result_stack(struct string_buffer_t* json_out, const size_t* stack) {
  sint result = 0;
  _sb_printf(json_out, "\"result_stack\": [");
  for (size_t i = 0; i < stbds_arrlenu(stack); ++i) {
    _sb_printf(json_out, "%zu, ", stack[i]);
  }

  _sb_try_chop_suffix(json_out, ", ");
  _sb_append_str(json_out, "]");

  return result;
}

sint eval_dump_json(struct string_buffer_t* json_out, EvalState state) {
  sint result = 0;
  _sb_append_str(json_out, "{\n");

  result = _eval_cells_dump_json(json_out, state->cells);
  CHECK(result == 0);
  _sb_append_str(json_out, ",\n");

  result = dump_apply_stack(json_out, state->apply_stack);
  CHECK(result == 0);
  _sb_append_str(json_out, ",\n");

  result = dump_result_stack(json_out, state->result_stack);
  CHECK(result == 0);
  _sb_append_str(json_out, ",\n");

  if (_sb_try_chop_suffix(json_out, ",\n")) {
    _sb_append_char(json_out, '\n');
  }
  _sb_append_char(json_out, '}');
cleanup:
  return result;
}

sint _eval_cells_dump_json(struct string_buffer_t* json_out, Allocator cells) {
  sint result = 0;
  char mappings[] = {'*', '^', '$', '#'};

  _sb_printf(json_out, "\"cells\": \"");
  size_t i = 0;
  while (eval_cells_is_set(cells, i)) {
    u8 cell = eval_cells_get(cells, i);
    _sb_append_char(json_out, mappings[cell]);
    i++;
  }
  _sb_printf(json_out, "\",\n");

  _sb_printf(json_out, "\"words\": [");
  i = 0;
  while (eval_cells_is_set(cells, i)) {
    sint i_word = eval_cells_get_word(cells, i);
    if (i_word != ERR_VAL) {
      sint i_cell = eval_cells_get(cells, i);
      if (i_cell == SIGIL_REF) {
        _sb_printf(json_out, "{ \"index\": %zu, \"ref\": %ld }, ", i, i_word);
      } else {
        u8 tag = eval_tv_get_tag(i_word);
        u64 payload = eval_tv_get_payload_unsigned(i_word);
        _sb_printf(
            json_out, "{ \"index\": %zu, \"tag\": %d, \"payload\": %zu }, ", i, tag, payload);
      }
    }
    i++;
  }
  _sb_try_chop_suffix(json_out, ", ");
  _sb_append_str(json_out, "]");

  return result;
}

#undef CHECK
