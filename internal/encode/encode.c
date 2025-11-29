
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "third_party/stb_ds/stb_ds.h"

#include "encode.h"
#include "internal/eval/eval_api.h"
#include "internal/json_parser/json_parser_api.h"
#include "internal/memory/bitmap_api.h"
#include "internal/memory/memory_api.h"
#include "internal/utility/logging_api.h"
#include "internal/utility/string_buffer_api.h"

#define CHECK_ERROR(on_error)                                                                      \
  if (err) {                                                                                       \
    on_error;                                                                                      \
    goto error;                                                                                    \
  }

static char CELL_TO_CHAR[] = {'*', '^', '#'};

void _eval_debug_dump(eval_state_t* state, string_buffer_t* buffer) {
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
  size_t* apply_stack = eval_get_apply_stack(state);                                               \
  for (size_t i = 0; i < stbds_arrlenu(apply_stack); ++i) {                                        \
    size_t value = apply_stack[i];                                                                 \
    if (value == TOKEN_APPLY) {                                                                    \
      _sb_printf(buffer, "%d ", -1);                                                               \
    } else {                                                                                       \
      _sb_printf(buffer, "%zu ", value);                                                           \
    }                                                                                              \
  }                                                                                                \
  _sb_append_char(buffer, '\n');                                                                   \
  _sb_printf(buffer, "result: ");                                                                  \
  size_t* result_stack = eval_get_result_stack(state);                                             \
  for (size_t i = 0; i < stbds_arrlenu(result_stack); ++i) {                                       \
    size_t value = result_stack[i];                                                                \
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
  allocator_t* cells = eval_get_cells(state);
  while (eval_cells_is_set(cells, i)) {
    if ((i % WINDOWS_LINE == 0) && i > 0) {
      DUMP_BUFFER
      _sb_clear(&indices_line);
      _sb_clear(&cells_line);
      _sb_clear(&words_line);
    }
    _sb_printf(&indices_line, num_format, i);
    u8 cell = eval_cells_get(cells, i);
    _sb_printf(&cells_line, char_format, CELL_TO_CHAR[cell]);
    // TODO: fix for natives
    if (cell == SIGIL_REF) {
      sint ref = 0;
      sint err = eval_cells_get_word(cells, i, &ref);
      if (err != ERR_VAL) {
        size_t ref_index = ref + i;
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

sint eval_load_json(const char* json, eval_state_t* state) {
  sint err = 0;

  json_parser_t parser = {};
  err = _json_parser_init(json, &parser);
  CHECK_ERROR({ logg_s("failed to parse json"); })
  err = _eval_load_json(&parser, state);
  CHECK_ERROR({ logg_s("failed to parse json"); })

error:
  return 0;
}

sint _eval_load_json(json_parser_t* parser, eval_state_t* state) {
  sint err = 0;

  eval_reset_errors(state);

  _JSON_PARSER_EAT(OBJECT, 1);
  _JSON_PARSER_EAT_KEY("cells", 1)
  if (_json_parser_match(parser, JSON_TOKEN_NULL)) {
    _JSON_PARSER_EAT(NULL, 1);
  } else {
    err = _eval_reset_cells(state);
    CHECK_ERROR({})
    err = _eval_cells_load_json(parser, state);
    CHECK_ERROR({})

    size_t i = 0;
    while (eval_cells_is_set(eval_get_cells(state), i)) {
      _bitmap_set_bit(eval_get_free_bitmap(state), i, 1);
      i++;
    }
  }

  size_t* apply_stack = eval_get_apply_stack(state);
  _JSON_PARSER_EAT_KEY("apply_stack", 1)
  if (_json_parser_match(parser, JSON_TOKEN_NULL)) {
    _JSON_PARSER_EAT(NULL, 1);
  } else {
    stbds_arrsetlen(apply_stack, 0);
    _JSON_PARSER_EAT(ARRAY, 1);
    size_t apply_count = parser->entries_count;
    for (size_t i = 0; i < apply_count; ++i) {
      _JSON_PARSER_EAT(INTEGER, 1);
      if (parser->digested_integer == -1) {
        stbds_arrpush(apply_stack, TOKEN_APPLY);
      } else {
        stbds_arrpush(apply_stack, parser->digested_integer);
      }
    }
  }

  size_t* result_stack = eval_get_result_stack(state);
  _JSON_PARSER_EAT_KEY("result_stack", 1)
  if (_json_parser_match(parser, JSON_TOKEN_NULL)) {
    _JSON_PARSER_EAT(NULL, 1);
  } else {
    stbds_arrsetlen(result_stack, 0);
    _JSON_PARSER_EAT(ARRAY, 1);
    size_t apply_count = parser->entries_count;
    for (size_t i = 0; i < apply_count; ++i) {
      _JSON_PARSER_EAT(INTEGER, 1);
      stbds_arrpush(result_stack, parser->digested_integer);
    }
  }

error:
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

sint _eval_cells_load_json(struct json_parser_t* parser, eval_state_t* state) {
  sint err = 0;
  allocator_t* cells = eval_get_cells(state);
  _JSON_PARSER_EAT(OBJECT, 1);
  _JSON_PARSER_EAT_KEY("state", 1)
  if (_json_parser_match(parser, JSON_TOKEN_NULL)) {
    _JSON_PARSER_EAT(NULL, 1);
  } else {
    _JSON_PARSER_EAT(STRING, 1);
    const char* state_str = _json_parser_get_string(parser);
    for (size_t i = 0; i < strlen(state_str); ++i) {
      eval_cells_set(cells, i, get_cell(state_str[i]));
    }
  }

  _JSON_PARSER_EAT_KEY("words", 1)
  if (_json_parser_match(parser, JSON_TOKEN_NULL)) {
    _JSON_PARSER_EAT(NULL, 1);
  } else {
    _JSON_PARSER_EAT(ARRAY, 1);
    size_t words_count = parser->entries_count;
    for (size_t i = 0; i < words_count; ++i) {
      _JSON_PARSER_EAT(OBJECT, 1);
      _JSON_PARSER_EAT_KEY("index", 1)
      _JSON_PARSER_EAT(INTEGER, 1);
      size_t index = parser->digested_integer;
      _JSON_PARSER_EAT_KEY("payload", 1)
      if (_json_parser_match(parser, JSON_TOKEN_INTEGER)) {
        _JSON_PARSER_EAT(INTEGER, 1);
        sint payload = parser->digested_integer;
        err = eval_cells_set_word(cells, index, payload);
        CHECK_ERROR({})
      } else if (_json_parser_match(parser, JSON_TOKEN_STRING)) {
        _JSON_PARSER_EAT(STRING, 1);
        uint symbol = 0;
        err = eval_get_native(state, _json_parser_get_string(parser), &symbol);
        CHECK_ERROR({})
        err = eval_cells_set_word(cells, index, symbol);
        CHECK_ERROR({})
      } else {
        assert(0 && "unreachable");
      }
    }
  }

error:
  return err;
}

// ********************** JSON DUMPING **********************

#define CHECK(cond)                                                                                \
  if (!(cond)) {                                                                                   \
    goto error;                                                                                    \
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

sint eval_dump_json(struct string_buffer_t* json_out, eval_state_t* state) {
  sint result = 0;
  _sb_append_str(json_out, "{\n");

  result = _eval_cells_dump_json(json_out, eval_get_cells(state));
  CHECK(result == 0);
  _sb_append_str(json_out, ",\n");

  result = dump_apply_stack(json_out, eval_get_apply_stack(state));
  CHECK(result == 0);
  _sb_append_str(json_out, ",\n");

  CHECK(result == 0);
  result = dump_result_stack(json_out, eval_get_result_stack(state));

  _sb_append_str(json_out, ",\n");

  if (_sb_try_chop_suffix(json_out, ",\n")) {
    _sb_append_char(json_out, '\n');
  }
  _sb_append_char(json_out, '}');
error:
  return result;
}

sint _eval_cells_dump_json(struct string_buffer_t* json_out, allocator_t* cells) {
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
    sint i_word = 0;
    sint err = eval_cells_get_word(cells, i, &i_word);
    if (err != ERR_VAL) {
      _sb_printf(json_out, "{ \"index\": %zu, \"payload\": %ld }, ", i, i_word);
    }
    i++;
  }
  _sb_try_chop_suffix(json_out, ", ");
  _sb_append_str(json_out, "]");

  return result;
}

#undef CHECK
