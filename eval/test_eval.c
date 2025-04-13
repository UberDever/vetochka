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

uint g_test_error = 0;
#define ERROR_BUF_SIZE 65536
static char g_error_buf[ERROR_BUF_SIZE] = {};

bool test_memory_smoke() {
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

bool test_memory_many_cells() {
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

bool test_encode_parse_smoke() {
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
      if (cell == ENCODE_NATIVE) {
        word_t word = eval_cells_get_word(cells, j);
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

#define ADD_TEST(test)                                                         \
  stbds_arrput(tests, test);                                                   \
  stbds_arrput(names, #test);

int main() {
  bool (**tests)() = NULL;
  const char **names = NULL;
  ADD_TEST(test_memory_smoke);
  ADD_TEST(test_memory_many_cells);
  ADD_TEST(test_encode_parse_smoke);

  for (int i = 0; i < stbds_arrlen(tests); i++) {
    g_test_error = 0; // Reset before each test
    g_error_buf[0] = '\0';
    if (tests[i]()) {
      printf("%s: PASSED\n", names[i]);
    } else {
      printf("%s: FAILED Error: %s\n", names[i], g_error_buf);
    }
  }

  stbds_arrfree(tests);
  stbds_arrfree(names);
  return 0;
}
