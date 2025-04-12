#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "stb_ds.h"

typedef enum {
  TEST_SUCCESS,              // Test passed
  TEST_ALLOCATE_CELL_FAILED, // Cell allocation failed
  TEST_ALLOCATE_WORD_FAILED, // Word allocation failed
  TEST_SET_CELL_FAILED,      // Setting cell value failed
  TEST_SET_WORD_FAILED,      // Setting word value failed
  TEST_GET_CELL_FAILED,      // Getting cell value failed
  TEST_GET_WORD_FAILED,      // Getting word value failed
  TEST_FREE_CELL_FAILED,     // Freeing cell failed
  TEST_FREE_WORD_FAILED,     // Freeing word failed
  TEST_NEXT_CELL_FAILED,     // Next cell navigation failed
  TEST_NEXT_WORD_FAILED,     // Next word navigation failed
  TEST_POOL_GROWTH_FAILED,   // Pool growth failed
  TEST_FREE_FAILED           // General free failure
} TestError;

// Global variable to track the error
TestError test_error = TEST_SUCCESS;

void test_eval() {
  EvalState s = NULL;
  uint *nodes = NULL;
  stbds_arrput(nodes, node_new_app(1, 4));
  stbds_arrput(nodes, node_new_tree(1, 2));
  stbds_arrput(nodes, node_new_tree(node_new_invalid(), node_new_invalid()));
  stbds_arrput(nodes, node_new_tree(node_new_invalid(), node_new_invalid()));
  stbds_arrput(nodes, node_new_tree(node_new_invalid(), node_new_invalid()));

  eval_init(&s, 0, nodes, stbds_arrlen(nodes));
  eval_eval(s);

  eval_free(&s);
  stbds_arrfree(nodes);

  /* assert(s.error_code == 0); */
  /* assert(s.error[0] == '\0'); */
}

bool test_smoke() {
  Allocator allocator;
  allocator_init(&allocator, 2); // Initialize with 2 words

  // Allocate a cell
  uint cell_idx = allocator_allocate(&allocator, 0); // 0 for cell
  if (cell_idx == (uint)-1) {
    test_error = TEST_ALLOCATE_CELL_FAILED;
    return false;
  }

  // Set cell value to 3 (binary 11)
  if (!allocator_set(&allocator, cell_idx, 3)) {
    test_error = TEST_SET_CELL_FAILED;
    return false;
  }

  // Verify cell value
  uint cell_value = allocator_get(&allocator, cell_idx);
  if (cell_value != 3) {
    test_error = TEST_GET_CELL_FAILED;
    return false;
  }

  // Allocate a word
  uint word_idx = allocator_allocate(&allocator, 1); // 1 for word
  if (word_idx == (uint)-1) {
    test_error = TEST_ALLOCATE_WORD_FAILED;
    return false;
  }

  // Set word value
  uint64_t test_word_value = 0x123456789ABCDEF0;
  if (!allocator_set(&allocator, word_idx, test_word_value)) {
    test_error = TEST_SET_WORD_FAILED;
    return false;
  }

  // Verify word value
  uint64_t word_value = allocator_get(&allocator, word_idx);
  if (word_value != test_word_value) {
    test_error = TEST_GET_WORD_FAILED;
    return false;
  }

  // Free allocations
  if (!allocator_free(&allocator, cell_idx)) {
    test_error = TEST_FREE_CELL_FAILED;
    return false;
  }
  if (!allocator_free(&allocator, word_idx)) {
    test_error = TEST_FREE_WORD_FAILED;
    return false;
  }

  allocator_destroy(&allocator);
  return true;
}

bool test_pool_growth_cell() {
  Allocator allocator;
  allocator_init(&allocator, 1); // 1 word (32 cells)

  // Allocate 32 cells
  uint cell_indices[32];
  for (int i = 0; i < 32; i++) {
    cell_indices[i] = allocator_allocate(&allocator, 0);
    if (cell_indices[i] == (uint)-1) {
      test_error = TEST_ALLOCATE_CELL_FAILED;
      return false;
    }
  }

  // Allocate one more cell to trigger growth
  uint extra_cell = allocator_allocate(&allocator, 0);
  if (extra_cell == (uint)-1) {
    test_error = TEST_ALLOCATE_CELL_FAILED;
    return false;
  }

  // Verify the extra cell is in a new word
  size_t extra_word =
      (extra_cell >> 5) & 0x7FFFFF; // Mask assumes 31-bit word index
  if (extra_word == 0) {
    test_error = TEST_POOL_GROWTH_FAILED;
    return false;
  }

  // Clean up
  for (int i = 0; i < 32; i++) {
    if (!allocator_free(&allocator, cell_indices[i])) {
      test_error = TEST_FREE_CELL_FAILED;
      return false;
    }
  }
  if (!allocator_free(&allocator, extra_cell)) {
    test_error = TEST_FREE_CELL_FAILED;
    return false;
  }

  allocator_destroy(&allocator);
  return true;
}

bool test_pool_growth_word() {
  Allocator allocator;
  allocator_init(&allocator, 1); // 1 word

  // Allocate a cell, marking the word as used for cells
  uint cell_idx = allocator_allocate(&allocator, 0);
  if (cell_idx == (uint)-1) {
    test_error = TEST_ALLOCATE_CELL_FAILED;
    return false;
  }

  // Allocate a word, forcing pool growth
  uint word_idx = allocator_allocate(&allocator, 1);
  if (word_idx == (uint)-1) {
    test_error = TEST_ALLOCATE_WORD_FAILED;
    return false;
  }

  // Verify the word is in a new word
  size_t word_word = (word_idx >> 5) & 0x7FFFFF;
  if (word_word == 0) {
    test_error = TEST_POOL_GROWTH_FAILED;
    return false;
  }

  // Clean up
  if (!allocator_free(&allocator, cell_idx)) {
    test_error = TEST_FREE_CELL_FAILED;
    return false;
  }
  if (!allocator_free(&allocator, word_idx)) {
    test_error = TEST_FREE_WORD_FAILED;
    return false;
  }

  allocator_destroy(&allocator);
  return true;
}

bool test_fragmented_allocation() {
  Allocator allocator;
  allocator_init(&allocator, 2); // 2 words

  // Allocate first cell
  uint cell1 = allocator_allocate(&allocator, 0);
  if (cell1 == (uint)-1) {
    test_error = TEST_ALLOCATE_CELL_FAILED;
    return false;
  }
  size_t cell1_word = (cell1 >> 5) & 0x7FFFFF;
  size_t cell1_cell = cell1 & 0x1F; // 5-bit cell index

  // Allocate a word
  uint word1 = allocator_allocate(&allocator, 1);
  if (word1 == (uint)-1) {
    test_error = TEST_ALLOCATE_WORD_FAILED;
    return false;
  }
  size_t word1_word = (word1 >> 5) & 0x7FFFFF;

  // Allocate second cell
  uint cell2 = allocator_allocate(&allocator, 0);
  if (cell2 == (uint)-1) {
    test_error = TEST_ALLOCATE_CELL_FAILED;
    return false;
  }
  size_t cell2_word = (cell2 >> 5) & 0x7FFFFF;
  size_t cell2_cell = cell2 & 0x1F;

  // Verify cell1 and cell2 are in the same word, with consecutive cell indices
  if (cell1_word != cell2_word || cell2_cell != cell1_cell + 1) {
    test_error = TEST_ALLOCATE_CELL_FAILED;
    return false;
  }

  // Verify word1 is in a different word
  if (word1_word == cell1_word) {
    test_error = TEST_ALLOCATE_WORD_FAILED;
    return false;
  }

  // Test navigation
  if (allocator_next_cell(&allocator, cell1) != cell2) {
    test_error = TEST_NEXT_CELL_FAILED;
    return false;
  }
  if (allocator_next_word(&allocator, cell1) != word1) {
    test_error = TEST_NEXT_WORD_FAILED;
    return false;
  }

  // Clean up
  if (!allocator_free(&allocator, cell1) ||
      !allocator_free(&allocator, word1) ||
      !allocator_free(&allocator, cell2)) {
    test_error = TEST_FREE_FAILED;
    return false;
  }

  allocator_destroy(&allocator);
  return true;
}

bool test_invalid_free() {
  Allocator allocator;
  allocator_init(&allocator, 1);

  // Attempt to free an unallocated cell
  uint fake_cell = 0;
  if (allocator_free(&allocator, fake_cell)) {
    test_error = TEST_FREE_CELL_FAILED;
    return false;
  }

  // Allocate a cell
  uint cell_idx = allocator_allocate(&allocator, 0);
  if (cell_idx == (uint)-1) {
    test_error = TEST_ALLOCATE_CELL_FAILED;
    return false;
  }

  // Attempt to free it as a word
  uint fake_word_idx = cell_idx | (1U << 31); // Set MSB
  if (allocator_free(&allocator, fake_word_idx)) {
    test_error = TEST_FREE_WORD_FAILED;
    return false;
  }

  // Free correctly
  if (!allocator_free(&allocator, cell_idx)) {
    test_error = TEST_FREE_CELL_FAILED;
    return false;
  }

  allocator_destroy(&allocator);
  return true;
}

bool test_set_unallocated() {
  Allocator allocator;
  allocator_init(&allocator, 1);

  // Attempt to set an unallocated cell
  uint fake_cell = 0;
  if (allocator_set(&allocator, fake_cell, 1)) {
    test_error = TEST_SET_CELL_FAILED;
    return false;
  }

  // Allocate a cell
  uint cell_idx = allocator_allocate(&allocator, 0);
  if (cell_idx == (uint)-1) {
    test_error = TEST_ALLOCATE_CELL_FAILED;
    return false;
  }

  // Set it correctly
  if (!allocator_set(&allocator, cell_idx, 2)) {
    test_error = TEST_SET_CELL_FAILED;
    return false;
  }

  // Attempt to set it as a word
  uint fake_word_idx = cell_idx | (1U << 31);
  if (allocator_set(&allocator, fake_word_idx, 0x123456789ABCDEF0)) {
    test_error = TEST_SET_WORD_FAILED;
    return false;
  }

  // Clean up
  if (!allocator_free(&allocator, cell_idx)) {
    test_error = TEST_FREE_CELL_FAILED;
    return false;
  }

  allocator_destroy(&allocator);
  return true;
}

int main() {
  bool(*tests[])() = {test_smoke,
                  test_pool_growth_cell,
                  test_pool_growth_word,
                  test_fragmented_allocation,
                  test_invalid_free,
                  test_set_unallocated};
  const char *names[] = {"Smoke Test",       "Pool Growth Cell",
                         "Pool Growth Word", "Fragmented Allocation",
                         "Invalid Free",     "Set Unallocated"};
  int num_tests = sizeof(tests) / sizeof(tests[0]);

  for (int i = 0; i < num_tests; i++) {
    test_error = TEST_SUCCESS; // Reset before each test
    if (tests[i]()) {
      printf("%s: PASSED\n", names[i]);
    } else {
      printf("%s: FAILED (Error: %d)\n", names[i], test_error);
    }
  }
  return 0;
}
