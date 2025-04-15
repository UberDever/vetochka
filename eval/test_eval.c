#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "common.h"
#include "stb_ds.h"

#define ASSERT_TRUE(x)                                                         \
  if (!(x)) {                                                                  \
    result = false;                                                            \
    sprintf(g_error_buf, "%s", #x);                                            \
    goto cleanup;                                                              \
  }

#define ERROR_BUF_SIZE 65536
static char g_error_buf[ERROR_BUF_SIZE] = {};

bool test_memory_smoke(void *_) {
  bool result = true;

  Allocator cells;
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
  ASSERT_TRUE(eval_cells_get_word(cells, 3) == 0xDEADBEEF);

  goto cleanup;

cleanup:
  eval_cells_free(&cells);
  return result;
}

typedef struct {
  uint8_t cell;
  word_t word;
} test_memory_many_cells_entry;

typedef struct {
  size_t key;
  test_memory_many_cells_entry value;
} test_memory_many_cells_set_cells;

bool test_memory_many_cells(void *_) {
  bool result = true;

  Allocator cells;
  eval_cells_init(&cells, 1);

  int seed = time(0);
  srand(seed);
  printf("seed: %d\n", seed);
  uint cell_count = 1024 * 1024;
  test_memory_many_cells_set_cells *set_cells = NULL;
  for (size_t i = 0; i < cell_count; ++i) {
    double cell_type = (double)rand() / RAND_MAX;
    uint8_t cell_val = cell_type / 0.25;
    eval_cells_set(cells, i, cell_val);
    if (cell_type > 0.75) {
      word_t word = rand();
      eval_cells_set_word(cells, i, word);
      stbds_hmput(set_cells, i,
                  ((test_memory_many_cells_entry){cell_val, word}));
    } else {
      stbds_hmput(
          set_cells, i,
          ((test_memory_many_cells_entry){.cell = cell_val, .word = -1}));
    }
  }

  for (size_t i = 0; i < stbds_hmlenu(set_cells); ++i) {
    uint idx = set_cells[i].key;
    uint cell_val = eval_cells_get(cells, idx);
    if (cell_val != set_cells[i].value.cell) {
      result = false;
      sprintf(g_error_buf, "cell_val %zu != [%zu] %zu", cell_val, idx,
              set_cells[i].key);
      goto cleanup;
    }
    if (set_cells[i].value.word != (word_t)-1) {
      word_t word = eval_cells_get_word(cells, idx);
      if (word != set_cells[i].value.word) {
        result = false;
        sprintf(g_error_buf, "word %zu != [%zu] %zu", word, idx,
                set_cells[i].value.word);
        goto cleanup;
      }
    }
  }

cleanup:
  eval_cells_free(&cells);
  stbds_hmfree(set_cells);
  return result;
}

bool test_encode_parse_smoke(void *_) {
  bool result = true;

  const char *programs[] = {
      "$ ^ ^** ^** ^**",             // ^ ^ ^ ^
      "# 10 *",                      // 10
      "$ ^ ^ ^** * # 10 * # 12345 *" // ^ (^ ^) 10 12345
  };

  Allocator cells;
  eval_cells_init(&cells, 4);
  for (size_t i = 0; i < sizeof(programs) / sizeof(*programs); ++i) {
    eval_cells_clear(cells);
    uint res = eval_encode_parse(cells, programs[i]);
    if (res != 0) {
      sprintf(g_error_buf, "failed to parse %s", programs[i]);
      result = false;
      goto cleanup;
    }
    size_t j = 0;
    while (eval_cells_is_set(cells, j)) {
      uint8_t cell = eval_cells_get(cells, j);
      printf("%hhu ", cell);
      if (cell == EVAL_NATIVE) {
        word_t word = eval_cells_get_word(cells, j);
        word_t payload = EVAL_GET_PAYLOAD(word);
        printf("[%zu] ", payload);
      }
      j++;
    }
    printf("\n");
  }

cleanup:
  eval_cells_free(&cells);
  return result;
}

bool compare_trees(Allocator lhs, Allocator rhs, size_t lhs_root,
                   size_t rhs_root) {
  uint lhs_cell = eval_cells_get(lhs, lhs_root);
  if (lhs_cell == ERROR_VALUE) {
    sprintf(g_error_buf, "invalid cell [%zu] %zu", lhs_root, lhs_cell);
    return false;
  }
  uint rhs_cell = eval_cells_get(rhs, rhs_root);
  if (rhs_cell == ERROR_VALUE) {
    sprintf(g_error_buf, "invalid cell [%zu] %zu", rhs_root, rhs_cell);
    return false;
  }
  if (lhs_cell != rhs_cell) {
    sprintf(g_error_buf, "cells differ at [%zu] [%zu] %zu != %zu", lhs_root,
            rhs_root, lhs_cell, rhs_cell);
    return false;
  }
  if (lhs_cell == EVAL_NATIVE) {
    word_t lhs_word = eval_cells_get_word(lhs, lhs_root);
    if (lhs_word == ERROR_VALUE) {
      sprintf(g_error_buf, "invalid value [%zu] %zu", lhs_root, lhs_word);
      return false;
    }
    word_t rhs_word = eval_cells_get_word(rhs, rhs_root);
    if (rhs_word == ERROR_VALUE) {
      sprintf(g_error_buf, "invalid value [%zu] %zu", rhs_root, rhs_word);
      return false;
    }
    if (lhs_word != rhs_word) {
      sprintf(g_error_buf, "words differ at [%zu] [%zu] %zu != %zu", lhs_root,
              rhs_root, lhs_word, rhs_word);
      return false;
    }
  }

  if (lhs_cell != EVAL_NIL) {
    bool result = false;
    result |= compare_trees(lhs, rhs, lhs_root + 1, rhs_root + 1);
    if (!result) {
      return result;
    }
    result |= compare_trees(lhs, rhs, lhs_root + 2, rhs_root + 2);
    if (!result) {
      return result;
    }
  }
  return true;
}

bool compare_results(Allocator lhs, size_t lhs_root, const char *expected) {
  bool result = true;
  Allocator rhs = NULL;
  eval_cells_init(&rhs, 4);
  uint res = eval_encode_parse(rhs, expected);
  if (res != 0) {
    sprintf(g_error_buf, "failed to parse %s", expected);
    result = false;
    goto cleanup;
  }
  if (!compare_trees(lhs, rhs, lhs_root, 0)) {
    result = false;
    goto cleanup;
  }

cleanup:
  eval_cells_free(&rhs);
  return result;
}

typedef struct {
  const char *program;
  const char *expected;
} test_eval_data;

bool test_eval_first_rule(void *data_ptr) {
  bool result = true;
  test_eval_data *data = data_ptr;
  EvalState state = NULL;
  const char *program = data->program;
  eval_init(&state, program);
  uint new_root = eval_step(state);
  const char* error_msg = "";
  uint8_t error_code = eval_get_error(state, &error_msg);
  if (error_code) {
    result = false;
    sprintf(
        g_error_buf,
        "failed to step for program '%s'\ncode: %d msg: '%s'",
        program,
        error_code,
        error_msg);
    goto cleanup;
  }

  Allocator cells = eval_get_memory(state);
  result = compare_results(cells, new_root, data->expected);

cleanup:
  eval_free(&state);
  return result;
}

#define ADD_TEST(test, datum)                                                  \
  stbds_arrput(tests, test);                                                   \
  stbds_arrput(names, #test " $ " #datum);                                     \
  stbds_arrput(data, datum);

int main() {
  bool (**tests)(void *) = NULL;
  const char **names = NULL;
  void **data = NULL;
  ADD_TEST(test_memory_smoke, NULL);
  ADD_TEST(test_memory_many_cells, NULL);
  ADD_TEST(test_encode_parse_smoke, NULL);

#define ADD_TEST_WITH_DATA(testcase, N, prog, exp)                                                 \
  test_eval_data testcase##_##N = {.program = prog, .expected = exp};                              \
  ADD_TEST(testcase, &testcase##_##N);

  ADD_TEST_WITH_DATA(test_eval_first_rule, 1, "$ ^ ^** ^** ^**", "^**");
  ADD_TEST_WITH_DATA(test_eval_first_rule, 2, "$ ^ ^** # 10 * *", "# 10 *");

  for (int i = 0; i < stbds_arrlen(tests); i++) {
    g_error_buf[0] = '\0';
    if (tests[i](data[i])) {
      printf("%s: PASSED\n", names[i]);
    } else {
      printf("%s: FAILED Error: %s\n", names[i], g_error_buf);
    }
  }

  stbds_arrfree(tests);
  stbds_arrfree(names);
  stbds_arrfree(data);
  return 0;
}
