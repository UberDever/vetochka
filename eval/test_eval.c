#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "common.h"
#include "stb_ds.h"

#define ASSERT_TRUE(x)                                                                             \
  if (!(x)) {                                                                                      \
    result = false;                                                                                \
    printf("%s", #x);                                                                              \
    goto cleanup;                                                                                  \
  }

bool test_memory_smoke(void* _) {
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
  sint word;
} test_memory_many_cells_entry;

typedef struct {
  size_t key;
  test_memory_many_cells_entry value;
} test_memory_many_cells_set_cells;

bool test_memory_many_cells(void* _) {
  bool result = true;

  Allocator cells;
  eval_cells_init(&cells, 1);

  int seed = time(0);
  srand(seed);
  printf("seed: %d\n", seed);
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
      printf("cell_val %zu != [%zu] %zu\n", cell_val, idx, set_cells[i].key);
      goto cleanup;
    }
    if (set_cells[i].value.word != (sint)-1) {
      sint word = eval_cells_get_word(cells, idx);
      if (word != set_cells[i].value.word) {
        result = false;
        printf("word %zu != [%zu] %zu\n", word, idx, set_cells[i].value.word);
        goto cleanup;
      }
    }
  }

cleanup:
  eval_cells_free(&cells);
  stbds_hmfree(set_cells);
  return result;
}

bool test_encode_parse_smoke(void* _) {
  bool result = true;

  const char* programs[] = {
      "^ ^** ^** ^**",             // ^ ^ ^ ^
      "# 10 *",                    // 10
      "^ ^ ^** * # 10 * # 12345 *" // ^ (^ ^) 10 12345
  };

  Allocator cells;
  eval_cells_init(&cells, 4);
  for (size_t i = 0; i < sizeof(programs) / sizeof(*programs); ++i) {
    eval_cells_clear(cells);
    sint res = eval_encode_parse(cells, programs[i]);
    if (res != 0) {
      printf("failed to parse %s", programs[i]);
      result = false;
      goto cleanup;
    }
    size_t j = 0;
    while (eval_cells_is_set(cells, j)) {
      uint8_t cell = eval_cells_get(cells, j);
      printf("%hhu ", cell);
      if (cell == EVAL_REF) {
        sint word = eval_cells_get_word(cells, j);
        printf("[%zu] ", word);
      }
      j++;
    }
    printf("\n");
  }

cleanup:
  eval_cells_free(&cells);
  return result;
}

#define EXPECT(cond, code, msg)                                                                    \
  if (!(cond)) {                                                                                   \
    printf(#cond " failed\n");                                                                     \
    goto failed;                                                                                   \
  }

static inline bool is_ref(Allocator cells, size_t index) {
  GET_CELL(cells, index)
  return (index_cell == EVAL_REF);
failed:
  return false;
}

// NOTE: this function could benefit from skip subtree feature
// since we don't always want to reference everything to everything in the expected, we only want to
// compare the nodes of the trees
bool compare_trees(Allocator lhs_cells, Allocator rhs_cells, size_t lhs, size_t rhs) {
  sint lhs_cell = eval_cells_get(lhs_cells, lhs);
  if (lhs_cell == ERR_VAL) {
    printf("invalid cell [%zu] %zu\n", lhs, lhs_cell);
    return false;
  }
  if (is_ref(lhs_cells, lhs)) {
    DEREF(lhs_cells, lhs)
    lhs_cell = eval_cells_get(lhs_cells, lhs);
    if (lhs_cell == ERR_VAL) {
      printf("invalid cell [%zu] %zu\n", lhs, lhs_cell);
      return false;
    }
  }

  sint rhs_cell = eval_cells_get(rhs_cells, rhs);
  if (rhs_cell == ERR_VAL) {
    printf("invalid cell [%zu] %zu\n", rhs, rhs_cell);
    return false;
  }
  if (is_ref(rhs_cells, rhs)) {
    DEREF(rhs_cells, rhs)
    rhs_cell = eval_cells_get(rhs_cells, rhs);
    if (rhs_cell == ERR_VAL) {
      printf("invalid cell [%zu] %zu\n", rhs, rhs_cell);
      return false;
    }
  }

  if (lhs_cell != rhs_cell) {
    printf("%ld[%zu] != %ld[%zu]\n", lhs_cell, lhs, rhs_cell, rhs);
    return false;
  }
  printf("%ld[%zu] == %ld[%zu]\n", lhs_cell, lhs, rhs_cell, rhs);
  if (lhs_cell == EVAL_REF) {
    sint lhs_word = eval_cells_get_word(lhs_cells, lhs);
    if (lhs_word == ERR_VAL) {
      printf("invalid value [%zu] %zu\n", lhs, lhs_word);
      return false;
    }
    sint rhs_word = eval_cells_get_word(rhs_cells, rhs);
    if (rhs_word == ERR_VAL) {
      printf("invalid value [%zu] %zu\n", rhs, rhs_word);
      return false;
    }
    if (lhs_word != rhs_word) {
      printf("words differ at [%zu] [%zu] %zu != %zu\n", lhs, rhs, lhs_word, rhs_word);
      return false;
    }
  }

  if (lhs_cell != EVAL_NIL) {
    bool result = true;
    result &= compare_trees(lhs_cells, rhs_cells, lhs + 1, rhs + 1);
    if (!result) {
      return result;
    }
    result &= compare_trees(lhs_cells, rhs_cells, lhs + 2, rhs + 2);
    if (!result) {
      return result;
    }
  }
  return true;

failed:
  return false;
}

static void dump_cells_and_stack(Allocator cells, Stack stack) {
  printf("stack: ");
  for (size_t i = 0; i < stbds_arrlenu(stack); ++i) {
    if (stack[i].type == StackEntryType_Index) {
      printf("%zu ", stack[i].as_index);
    } else if (stack[i].type == StackEntryType_Calculated) {
      printf("$%u ", stack[i].as_calculated_index.type);
    } else {
      assert(false);
    }
  }
  printf("\n");
  eval_encode_dump(cells, 0);
}

static bool compare_results(Allocator lhs, Stack* lhs_stack, Allocator rhs, Stack* rhs_stack) {
  bool result = true;

  size_t lhs_size = stbds_arrlenu(*lhs_stack);
  size_t rhs_size = stbds_arrlenu(*rhs_stack);
  if (lhs_size == 0 && rhs_size == 0) {
    stbds_arrput(*lhs_stack, ((StackEntry){.type = StackEntryType_Index, .as_index = 0}));
    stbds_arrput(*rhs_stack, ((StackEntry){.type = StackEntryType_Index, .as_index = 0}));
  }
  if (lhs_size != rhs_size) {
    result = false;
    printf("stack sizes are different: %zu != %zu\n", lhs_size, rhs_size);
    goto cleanup;
  }

  dump_cells_and_stack(lhs, *lhs_stack);
  dump_cells_and_stack(rhs, *rhs_stack);

  for (size_t i = 0; i < lhs_size; ++i) {
    StackEntry lhs_entry = stbds_arrpop(*lhs_stack);
    StackEntry rhs_entry = stbds_arrpop(*rhs_stack);
    if (lhs_entry.type == StackEntryType_Calculated
        && rhs_entry.type == StackEntryType_Calculated) {
      result = lhs_entry.as_calculated_index.type == rhs_entry.as_calculated_index.type;
      if (!result) {
        printf(
            "different calculated indices %d %d\n",
            lhs_entry.as_calculated_index.type,
            rhs_entry.as_calculated_index.type);
        goto cleanup;
      }
      continue;
    }

    if (lhs_entry.type == StackEntryType_Index && rhs_entry.type == StackEntryType_Index) {
      size_t lhs_root = lhs_entry.as_index;
      size_t rhs_root = rhs_entry.as_index;
      if (!compare_trees(lhs, rhs, lhs_root, rhs_root)) {
        printf("program state, root = %zu:\n", lhs_root);
        eval_encode_dump(lhs, 0);
        printf("expected state, root = %zu:\n", rhs_root);
        eval_encode_dump(rhs, 0);
        result = false;
        goto cleanup;
      }
      continue;
    }

    printf("uncomparable indices %d %d\n", lhs_entry.type, rhs_entry.type);
    result = false;
    goto cleanup;
  }

cleanup:
  return result;
}

typedef struct {
  const char* program;
  const char* expected_stack;
  const char* expected;
  i64 steps;
} test_eval_data;

bool parse_test_stack(const char* stack, Stack* out) {
  bool result = true;
  const char* delimiters = " \t\n";
  char* prog = malloc(sizeof(char) * (strlen(stack) + 1));
  strcpy(prog, stack);
  char* token = strtok(prog, delimiters);
  while (token != NULL) {
    if (!strcmp(token, "r2")) {
      stbds_arrput(
          *out,
          ((StackEntry){.type = StackEntryType_Calculated,
                        .as_calculated_index = {.type = CalculatedIndexType_Rule2}}));
      token = strtok(NULL, delimiters);
      continue;
    }
    if (!strcmp(token, "r3c")) {
      stbds_arrput(
          *out,
          ((StackEntry){.type = StackEntryType_Calculated,
                        .as_calculated_index = {.type = CalculatedIndexType_Rule3c}}));
      token = strtok(NULL, delimiters);
      continue;
    }

    char* endptr = NULL;
    size_t value = strtoull(token, &endptr, 10);
    if (*endptr != '\0') {
      result = false;
      goto cleanup;
    }
    stbds_arrput(*out, ((StackEntry){.type = StackEntryType_Index, .as_index = value}));
    token = strtok(NULL, delimiters);
  }
cleanup:
  free(prog);
  return result;
}

bool prepare_expected(Allocator* expected_cells, Stack* expected_stack, test_eval_data* data) {
  bool result = true;
  if (!parse_test_stack(data->expected_stack, expected_stack)) {
    result = false;
    printf("failed to parse test stack %s\n", data->expected_stack);
    goto cleanup;
  }

  eval_cells_init(expected_cells, 4);
  sint res = eval_encode_parse(*expected_cells, data->expected);
  if (res != 0) {
    result = false;
    printf("failed to parse %s\n", data->expected);
    goto cleanup;
  }

cleanup:
  return result;
}

bool test_eval_eval(void* data_ptr) {
  bool result = true;
  test_eval_data* data = data_ptr;
  // TODO: use step counter

  Stack expected_stack = NULL;
  Allocator expected_cells = NULL;
  if (!prepare_expected(&expected_cells, &expected_stack, data)) {
    result = false;
    goto cleanup;
  }

  EvalState state = NULL;
  const char* program = data->program;
  eval_init(&state, program);
  eval_step(state);

  const char* error_msg = "";
  Allocator cells = eval_get_memory(state);
  Stack stack = eval_get_stack(state);
  uint8_t error_code = eval_get_error(state, &error_msg);
  if (error_code) {
    result = false;
    printf("failed to step for program '%s'\ncode: %d msg: '%s'\n", program, error_code, error_msg);
    goto cleanup;
  }

  result = compare_results(cells, &stack, expected_cells, &expected_stack);

cleanup:
  stbds_arrfree(expected_stack);
  eval_cells_free(&expected_cells);
  eval_free(&state);
  return result;
}

#define ADD_TEST(test, datum)                                                                      \
  stbds_arrput(tests, test);                                                                       \
  stbds_arrput(names, #test " $ " #datum);                                                         \
  stbds_arrput(data, datum);

// TODO: it would be nice if we added a evaluation step support
// i.e. describe input, describe expected steps of evaluation (and IO state presumably), describe
// step count (too much for C, use different language?)
int main() {
  int result = 0;
  bool (**tests)(void*) = NULL;
  const char** names = NULL;
  void** data = NULL;
  ADD_TEST(test_memory_smoke, NULL);
  ADD_TEST(test_memory_many_cells, NULL);
  ADD_TEST(test_encode_parse_smoke, NULL);

#define ADD_TEST_WITH_DATA(testcase, N, prog, roots, exp, steps_n)                                 \
  test_eval_data testcase##_data_##N = {                                                           \
      .program = prog, .expected_stack = roots, .expected = exp, .steps = steps_n};                \
  ADD_TEST(testcase, &testcase##_data_##N);

  // pure data
  ADD_TEST_WITH_DATA(test_eval_eval, 0, "^ ^** ^**", "0", "^ ^** ^**", 1);

  // first rule
  ADD_TEST_WITH_DATA(test_eval_eval, 1, "^ ^** # 2 # 3 ^** ^**", "0", "^**", 1);
  ADD_TEST_WITH_DATA(test_eval_eval, 2, "^ ^** # 2 # 3 ^** *", "0", "^**", 1);
  ADD_TEST_WITH_DATA(test_eval_eval, 3, "^ ^** # 2 # 3 ^** *", "0", "^**", -1);

  // second rule
  ADD_TEST_WITH_DATA(
      test_eval_eval,
      4,
      "^ ^ # 4 * # 5 # 9 ^** ^^*** ^^**^**",
      "21 23 r2",
      "^ ^ # 4 * # 5 # 9 ^** ^^*** ^^**^** "
      "# -15 # -8 # -14 # -10",
      1);

  const char* GREEN = "\033[0;32m";
  const char* CYAN = "\033[0;36m";
  const char* RED = "\033[0;31m";
  const char* RESET = "\033[0m";
  for (int i = 0; i < stbds_arrlen(tests); i++) {
    printf("%s%s%s\n", CYAN, names[i], RESET);
    if (tests[i](data[i])) {
      printf("%sPASSED%s\n\n", GREEN, RESET);
    } else {
      printf("%sFAILED%s\n\n", RED, RESET);
      result = 1;
      goto cleanup;
    }
  }

cleanup:
  stbds_arrfree(tests);
  stbds_arrfree(names);
  stbds_arrfree(data);
  return result;
}
