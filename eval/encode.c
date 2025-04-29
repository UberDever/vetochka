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
  _JSON_PARSER_EAT(OBJECT, 1);
  _JSON_PARSER_EAT_KEY("cells", 1)

  err = _eval_cells_load_json(parser, state->cells);
  if (err) {
    goto cleanup;
  }

  _JSON_PARSER_EAT_KEY("control_stack", 1)
  _JSON_PARSER_EAT(ARRAY, 1);
  size_t control_count = parser->entries_count;
  for (size_t i = 0; i < control_count; ++i) {
    if (_json_parser_match(parser, JSON_TOKEN_NUMBER)) {
      _JSON_PARSER_EAT(NUMBER, 1);
      _eval_control_stack_push_index(state, parser->digested_number);
      continue;
    }
    _JSON_PARSER_EAT(OBJECT, 1);
    _JSON_PARSER_EAT_KEY("compute_index", 1)
    _JSON_PARSER_EAT(STRING, 1);
    if (strcmp(_json_parser_get_string(parser), "rule2") == 0) {
      _eval_control_stack_push_calculated(state, CALCULATED_INDEX_RULE2);
      continue;
    }
    if (strcmp(_json_parser_get_string(parser), "rule3c") == 0) {
      _eval_control_stack_push_calculated(state, CALCULATED_INDEX_RULE3C);
      continue;
    }
    err = 1;
    goto cleanup;
  }

  _JSON_PARSER_EAT_KEY("value_stack", 1)
  _JSON_PARSER_EAT(ARRAY, 1);
  size_t value_count = parser->entries_count;
  for (size_t i = 0; i < value_count; ++i) {
    _JSON_PARSER_EAT(NUMBER, 1);
    _eval_value_stack_push(state, parser->digested_number);
  }

cleanup:
  return err;
}

static u8 get_cell(char symbol) {
  if (symbol == '*') {
    return 0;
  }
  if (symbol == '^') {
    return 1;
  }
  if (symbol == '$') {
    return 2;
  }
  if (symbol == '#') {
    return 3;
  }
  assert(false && "unreachable");
}

sint _eval_cells_load_json(struct json_parser_t* parser, Allocator cells) {
  sint err = 0;
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

    _JSON_PARSER_EAT_KEY("index", 1)
    _JSON_PARSER_EAT(NUMBER, 1);
    size_t val_index = parser->digested_number;
    _JSON_PARSER_EAT_KEY("tag", 1)
    _JSON_PARSER_EAT(NUMBER, 1);
    u8 val_tag = parser->digested_number;
    _JSON_PARSER_EAT_KEY("payload", 1)
    _JSON_PARSER_EAT(NUMBER, 1);
    u8 val_payload = parser->digested_number;
    err = eval_cells_set_word(
        cells, val_index, eval_tv_new_tagged_value_signed(val_tag, val_payload));
    if (err) {
      goto cleanup;
    }
  }

cleanup:
  return err;
}

static int ENCODE_MAP[] = {
    ['*'] = EVAL_NIL,
    ['^'] = EVAL_TREE,
    //['$'] = EVAL_APPLY,
    ['#'] = EVAL_REF,
};

static bool char_is_node(char c) {
  return c == '*' || c == '^' /*|| c == '$'*/ || c == '#';
}

sint eval_encode_parse(Allocator cells, const char* program) {
  const char* delimiters = " \t\n";
  int result = 0;
  char* prog = malloc(sizeof(char) * (strlen(program) + 1));
  strcpy(prog, program);
  char* token = strtok(prog, delimiters);
  size_t index = 0;
  while (token != NULL) {
    if (char_is_node(token[0])) {
      for (size_t j = 0; j < strlen(token); ++j) {
        if (!char_is_node(token[j])) {
          result = -1;
          goto cleanup;
        }
        sint res = eval_cells_set(cells, index++, ENCODE_MAP[(size_t)token[j]]);
        if (res == ERR_VAL) {
          result = -1;
          goto cleanup;
        }
      }
    } else {
      char* endptr = NULL;
      sint value = strtoll(token, &endptr, 10);
      if (*endptr != '\0') {
        result = -1;
        goto cleanup;
      }

      if (index == 0) {
        result = -1;
        goto cleanup;
      }
      sint node = eval_cells_get(cells, index - 1);
      if (node == EVAL_REF) {
        // u8 tag = EVAL_TAG_INDEX;
        // u64 val = eval_tv_new_tagged_value_signed(tag, value);
        sint res = eval_cells_set_word(cells, index - 1, value);
        if (res == ERR_VAL) {
          result = -1;
          goto cleanup;
        }
      } else {
        result = -1;
        goto cleanup;
      }
      node = -1;
    }

    token = strtok(NULL, delimiters);
  }

cleanup:
  free(prog);
  return result;
}

void eval_encode_dump(Allocator cells, size_t root) {
  char nodes[] = {'*', '^', '$', '#'};
  size_t index = root;
  logg_s("nodes: ");
  while (eval_cells_is_set(cells, index)) {
    sint index_cell = eval_cells_get(cells, index);
    assert(index_cell != ERR_VAL);
    logg("%c[%zu] ", nodes[index_cell], index);
    index++;
  }
  logg_s("\n");

  logg_s("words: ");
  size_t word_index = root;
  while (eval_cells_is_set(cells, word_index)) {
    sint word_index_cell = eval_cells_get(cells, word_index);
    assert(word_index_cell != ERR_VAL);
    if (word_index_cell == EVAL_REF) {
      sint word_index_word = eval_cells_get_word(cells, word_index);
      assert(word_index_word != ERR_VAL);
      logg("!%ld[%zu] ", word_index_word, word_index);
      word_index++;
      continue;
    }
    word_index++;
  }
  logg_s("\n");
}

// ********************** JSON DUMPING **********************

#define CHECK(cond)                                                                                \
  if (!(cond)) {                                                                                   \
    goto cleanup;                                                                                  \
  }

static sint dump_control_stack(struct string_buffer_t* json_out, Stack stack) {
  sint result = 0;
  const char* mappings[] = {"INVALID", "rule2", "rule3c"};
  _sb_printf(json_out, "\"control_stack\": [");
  for (size_t i = 0; i < stbds_arrlenu(stack); ++i) {
    struct StackEntry e = stack[i];
    if (e.tag == STACK_ENTRY_INDEX) {
      _sb_printf(json_out, "%d, ", e.as_index);
    } else if (e.tag == STACK_ENTRY_CALCULATED) {
      _sb_printf(json_out, "{ \"compute_index\": \"%s\" }, ", mappings[(size_t)e.as_calculated]);
    } else {
      assert(false);
    }
  }
  _sb_try_chop_suffix(json_out, ", ");
  _sb_append_str(json_out, "]");

  return result;
}

static sint dump_value_stack(struct string_buffer_t* json_out, size_t* stack) {
  sint result = 0;
  _sb_printf(json_out, "\"value_stack\": [");
  for (size_t i = 0; i < stbds_arrlenu(stack); ++i) {
    _sb_printf(json_out, "%d, ", stack[i]);
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

  result = dump_control_stack(json_out, state->control_stack);
  CHECK(result == 0);
  _sb_append_str(json_out, ",\n");

  result = dump_value_stack(json_out, state->value_stack);
  CHECK(result == 0);
  _sb_append_str(json_out, ",\n");

  // TODO: rest of the json scheme

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
      if (i_cell == EVAL_REF) {
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
