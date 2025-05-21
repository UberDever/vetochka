
#include "api.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "vendor/stb_ds.h"

#include "config.h"
#include "encode.h"
#include "eval.h"
#include "util.h"

#ifndef PROJECT_ROOT
#error "must be defined in the config.h"
#endif
#ifndef PATH_SEP
#error "must be defined in the config.h"
#endif

#define ASSERT_TRUE(x)                                                                             \
  if (!(x)) {                                                                                      \
    result = false;                                                                                \
    printf("%s", #x);                                                                              \
    goto error;                                                                                    \
  }

#define STR(x) #x

typedef struct test_data_t {
  enum { test_data_none, test_data_json, test_data_file_testsuite } tag;

  union {
    const char* as_json;

    struct file_testsuite_t {
      const char* name;
      bool (*test)(struct test_data_t);

    } as_file_testsuite_t;
  };

  const char* name;
} test_data_t;

typedef bool (*test_t)(test_data_t);

typedef struct {
  test_t test;
  const char* name;
  struct test_data_t data;
} test_case_t;

void add_case(test_case_t** cases, test_t test, const char* name, test_data_t data) {
  stbds_arrput(*cases, ((test_case_t){.test = test, .name = name, .data = data}));
}

bool load_and_execute_testsuite(test_data_t data) {
  assert(data.tag == test_data_file_testsuite);
  const char* path = data.as_file_testsuite_t.name;

  string_buffer_t path_json;
  _sb_init(&path_json);
  _sb_append_str(&path_json, PROJECT_ROOT);
  _sb_append_str(&path_json, PATH_SEP);
  _sb_append_str(&path_json, "tests");
  _sb_append_str(&path_json, PATH_SEP);
  _sb_append_str(&path_json, path);
  _sb_append_str(&path_json, ".json");

  logg("suite %s", _sb_str_view(&path_json));

  string_buffer_t file_contents;
  _sb_init(&file_contents);

  bool test_result = false;

  FILE* file_json = fopen(_sb_str_view(&path_json), "r");
  if (file_json == NULL) {
    perror(_sb_str_view(&path_json));
    goto error;
  }

  char chunk[1024];
  while (fgets(chunk, sizeof(chunk), file_json) != NULL) {
    _sb_append_str(&file_contents, chunk);
  }

  if (ferror(file_json)) {
    perror(_sb_str_view(&path_json));
    fclose(file_json);
    goto error;
  }

  const char* testsuite = _sb_str_view(&file_contents);

  test_result =
      data.as_file_testsuite_t.test((test_data_t){.tag = test_data_json, .as_json = testsuite});

  fclose(file_json);
error:
  _sb_free(&path_json);
  _sb_free(&file_contents);
  return test_result;
}

#define UPDATE_RESULT(cond)                                                                        \
  result &= (cond);                                                                                \
  if (!result) {                                                                                   \
    logg_s(#cond " failed");                                                                       \
  }

#define EXPECT(cond, code, msg)                                                                    \
  if (!(cond)) {                                                                                   \
    logg_s(#cond " failed");                                                                       \
    goto failed;                                                                                   \
  }

bool compare_trees(eval_state_t* lhs_state, eval_state_t* rhs_state, size_t lhs, size_t rhs) {
  sint lhs_cell = eval_cells_get(lhs_state->cells, lhs);
  if (lhs_cell == ERR_VAL) {
    logg("invalid cell [%zu] %ld", lhs, lhs_cell);
    return false;
  }
  sint rhs_cell = eval_cells_get(rhs_state->cells, rhs);
  if (rhs_cell == ERR_VAL) {
    logg("invalid cell [%zu] %ld", rhs, rhs_cell);
    return false;
  }

  // NOTE: single nil node
  if (lhs_cell == SIGIL_NIL || rhs_cell == SIGIL_NIL) {
    return lhs_cell == rhs_cell;
  }

  bool result = true;

  if (_eval_is_terminal(lhs_state, lhs)) {
    if (!_eval_is_terminal(rhs_state, rhs)) {
      logg("%ld[%zu] != %ld[%zu]", lhs_cell, lhs, rhs_cell, rhs);
      return false;
    }

    sint lhs_left = eval_cells_get(lhs_state->cells, lhs + 1);
    sint lhs_right = eval_cells_get(lhs_state->cells, lhs + 2);
    sint rhs_left = eval_cells_get(lhs_state->cells, rhs + 1);
    sint rhs_right = eval_cells_get(lhs_state->cells, rhs + 2);
    UPDATE_RESULT(lhs_cell == rhs_cell);
    UPDATE_RESULT(lhs_left == rhs_left);
    UPDATE_RESULT(lhs_right == rhs_right);

    return result;
  }

  size_t lhs_left = _eval_get_left_node(lhs_state, lhs);
  size_t rhs_left = _eval_get_left_node(rhs_state, rhs);
  UPDATE_RESULT(compare_trees(lhs_state, rhs_state, lhs_left, rhs_left));

  size_t lhs_right = _eval_get_right_node(lhs_state, lhs);
  size_t rhs_right = _eval_get_right_node(rhs_state, rhs);
  UPDATE_RESULT(compare_trees(lhs_state, rhs_state, lhs_right, rhs_right));

  return result;
}

#undef UPDATE_RESULT

bool compare_stacks(size_t* actual, size_t* expected) {
  size_t lhs_size = stbds_arrlenu(actual);
  size_t rhs_size = stbds_arrlenu(expected);
  if (lhs_size != rhs_size) {
    logg("%zu != %zu", lhs_size, rhs_size);
    return false;
  }
  for (size_t i = 0; i < lhs_size; ++i) {
    if (actual[i] != expected[i]) {
      logg("[%zu] %zu != %zu", i, actual[i], expected[i]);
      return false;
    }
  }
  return true;
}

bool compare_states(eval_state_t* actual, eval_state_t* expected) {
  if (!compare_trees(actual, expected, 0, 0)) {
    return false;
  }
  if (!compare_stacks(actual->apply_stack, expected->apply_stack)) {
    return false;
  }
  if (!compare_stacks(actual->result_stack, expected->result_stack)) {
    return false;
  }

  return true;
}

// ********************** TESTCASES **********************

bool test_memory_smoke(test_data_t _) {
  bool result = true;

  allocator_t* cells;
  eval_cells_init(&cells, 10);

  size_t idx = 0;
  eval_cells_set(cells, idx++, 0);
  eval_cells_set(cells, idx++, 1);
  eval_cells_set(cells, idx++, 2);
  eval_cells_set(cells, idx++, 3);
  eval_cells_set_word(cells, idx - 1, 0xDEADBEEF);

  ASSERT_TRUE(eval_cells_get(cells, 0) == 0);
  ASSERT_TRUE(eval_cells_get(cells, 1) == 1);
  ASSERT_TRUE(eval_cells_get(cells, 2) == 2);
  ASSERT_TRUE(eval_cells_get(cells, 3) == 3);
  sint word = 0;
  sint err = eval_cells_get_word(cells, 3, &word);
  ASSERT_TRUE(err != -1);
  ASSERT_TRUE(word == 0xDEADBEEF);

  goto error;

error:
  eval_cells_free(&cells);
  return result;
}

typedef struct {
  uint8_t cell;
  sint word;
} test_memory_many_cells_entry;

typedef struct {
  size_t key;
  test_memory_many_cells_entry value;
} test_memory_many_cells_set_cells;

bool test_memory_many_cells(test_data_t _) {
  bool result = true;

  allocator_t* cells;
  eval_cells_init(&cells, 1);

  int seed = time(0);
  srand(seed);
  logg("seed: %d", seed);
  size_t cell_count = 1024 * 1024;
  test_memory_many_cells_set_cells* set_cells = NULL;
  for (size_t i = 0; i < cell_count; ++i) {
    double cell_type = (double)rand() / RAND_MAX;
    uint8_t cell_val = cell_type / 0.25;
    eval_cells_set(cells, i, cell_val);
    if (cell_type > 0.75) {
      sint word = rand();
      eval_cells_set_word(cells, i, word);
      stbds_hmput(set_cells, i, ((test_memory_many_cells_entry){cell_val, word}));
    } else {
      stbds_hmput(set_cells, i, ((test_memory_many_cells_entry){.cell = cell_val, .word = -1}));
    }
  }

  for (size_t i = 0; i < stbds_hmlenu(set_cells); ++i) {
    size_t idx = set_cells[i].key;
    sint cell_val = eval_cells_get(cells, idx);
    if (cell_val != set_cells[i].value.cell) {
      result = false;
      logg("cell_val %zu != [%zu] %zu", cell_val, idx, set_cells[i].key);
      goto error;
    }
    if (set_cells[i].value.word != (sint)-1) {
      sint word = 0;
      sint err = eval_cells_get_word(cells, idx, &word);
      if (err == -1 || word != set_cells[i].value.word) {
        result = false;
        logg("word %zu != [%zu] %zu", word, idx, set_cells[i].value.word);
        goto error;
      }
    }
  }

error:
  eval_cells_free(&cells);
  stbds_hmfree(set_cells);
  return result;
}

bool test_eval(test_data_t data) {
  sint err = 0;
  assert(data.tag == test_data_json);
  const char* json = data.as_json;
  eval_state_t* state = NULL;
  eval_state_t* reference_state = NULL;

  json_parser_t parser_ = {};
  json_parser_t* parser = &parser_;

  err = eval_init(&state);
  CHECK_ERROR({ logg_s("failed eval_init"); })
  err = eval_init(&reference_state);
  CHECK_ERROR({ logg_s("failed eval_init"); })
  native_load_standard(state);
  native_load_standard(reference_state);

  _json_parser_init(json, parser);
  _JSON_PARSER_EAT(ARRAY, 1);
  size_t cases_count = parser->entries_count;

  string_buffer_t debug_buffer;
  _sb_init(&debug_buffer);

  for (size_t case_num = 0; case_num < cases_count; case_num++) {
    _JSON_PARSER_EAT(OBJECT, 1);

    _JSON_PARSER_EAT_KEY("name", 1);
    _JSON_PARSER_EAT(STRING, 1);
    const char* case_name = _json_parser_get_string(parser);
    logg("case %zu: %s", case_num, case_name);

    bool output_final = false;
    _JSON_PARSER_EAT_KEY("output_final", 1);
    _JSON_PARSER_EAT(BOOL, 1);
    output_final = parser->digested_bool;

    _JSON_PARSER_EAT_KEY("input", 1);

    err = _eval_load_json(parser, state);
    CHECK_ERROR({ logg_s("failed eval_load_json"); })

    sint fully_evaluated = 0;

    if (output_final) {
      while (true) {
        fully_evaluated = eval_step(state);
        if (state->error_code) {
          logg("%s", state->error);
          err = state->error_code;
          goto dump_states;
        }

        if (fully_evaluated) {
          break;
        }
      }

      _JSON_PARSER_EAT_KEY("output", 1);
      _JSON_PARSER_EAT(ARRAY, 1);
      err = _eval_load_json(parser, reference_state);
      CHECK_ERROR({ logg_s("failed eval_load_json"); })
      if (!compare_states(state, reference_state)) {
        err = 1;
        logg_s("states are not equal");
        goto dump_states;
      }

      goto error;
    }

    _JSON_PARSER_EAT_KEY("output", 1);
    _JSON_PARSER_EAT(ARRAY, 1);
    size_t output_count = parser->entries_count;
    for (size_t step_count = 0; step_count < output_count; ++step_count) {
      err = _eval_load_json(parser, reference_state);
      CHECK_ERROR({ logg_s("failed eval_load_json"); })

      fully_evaluated = eval_step(state);
      if (state->error_code) {
        logg("%s", state->error);
        err = state->error_code;
        goto dump_states;
      }

      if (!compare_states(state, reference_state)) {
        err = 1;
        logg_s("states are not equal");
        goto dump_states;
      }
    }
    if (!fully_evaluated) {
      err = 1;
      logg_s("not fully evaluated");
      goto dump_states;
    }

    // _eval_debug_dump(state, &debug_buffer);
    // printf("%s", _sb_str_view(&debug_buffer));
    // _sb_clear(&debug_buffer);
  }

  goto error;

dump_states:
  logg_s("got:");
  _eval_debug_dump(state, &debug_buffer);
  printf("%s", _sb_str_view(&debug_buffer));
  _sb_clear(&debug_buffer);
  logg_s("expected:");
  _eval_debug_dump(reference_state, &debug_buffer);
  printf("%s", _sb_str_view(&debug_buffer));
  _sb_clear(&debug_buffer);

error:
  _sb_free(&debug_buffer);
  _json_parser_free(parser);
  eval_free(&state);
  eval_free(&reference_state);
  return err == 0;
}

#define add_file_case(casename)                                                                    \
  add_case(                                                                                        \
      &cases,                                                                                      \
      load_and_execute_testsuite,                                                                  \
      (casename),                                                                                  \
      (test_data_t){                                                                               \
          .tag = test_data_file_testsuite,                                                         \
          .as_file_testsuite_t = (struct file_testsuite_t){.name = (casename), .test = test_eval}, \
          .name = (casename)})

int main() {
  // NOTE: because of ninja
  stderr = stdout;

  int result = 0;
  test_case_t* cases = NULL;
  void* testsuite_data = NULL;

  add_case(
      &cases,
      test_memory_smoke,
      STR(test_memory_smoke),
      (test_data_t){.name = STR(test_memory_smoke)});
  add_case(
      &cases,
      test_memory_many_cells,
      STR(test_memory_many_cells),
      (test_data_t){.name = STR(test_memory_many_cells)});

  add_file_case("eval-smoke");
  add_file_case("eval-native");

  const char* GREEN = "\033[0;32m";
  const char* CYAN = "\033[0;36m";
  const char* RED = "\033[0;31m";
  const char* RESET = "\033[0m";
  for (int i = 0; i < stbds_arrlen(cases); i++) {
    printf("%s%s%s\n", CYAN, cases[i].name, RESET);
    if (cases[i].test(cases[i].data)) {
      printf("%sPASSED%s\n\n", GREEN, RESET);
    } else {
      printf("%sFAILED%s\n\n", RED, RESET);
      result = 1;
    }
  }

  if (result) {
    printf("%sYou have failed tests :(%s\n\n", RED, RESET);
  }

  stbds_arrfree(cases);
  stbds_arrfree(testsuite_data);
  return result;
}
