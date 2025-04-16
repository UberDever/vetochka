#include "common.h"
#include "stb_ds.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static int ENCODE_MAP[] = {
    ['*'] = EVAL_NIL,
    ['^'] = EVAL_TREE,
    ['$'] = EVAL_APPLY,
    ['#'] = EVAL_NATIVE,
};

static bool char_is_node(char c) {
  return c == '*' || c == '^' || c == '$' || c == '#';
}

uint eval_encode_parse(Allocator cells, const char *program) {
  const char *delimiters = " \t\n";
  int result = 0;
  char *prog = malloc(sizeof(char) * (strlen(program) + 1));
  strcpy(prog, program);
  char *token = strtok(prog, delimiters);
  size_t index = 0;
  while (token != NULL) {
    if (char_is_node(token[0])) {
      for (size_t j = 0; j < strlen(token); ++j) {
        if (!char_is_node(token[j])) {
          result = -1;
          goto cleanup;
        }
        uint res = eval_cells_set(cells, index++, ENCODE_MAP[(size_t)token[j]]);
        if (res == (uint)-1) {
          result = -1;
          goto cleanup;
        }
      }
    } else {
      char *endptr = NULL;
      bool is_index = false;
      if (token[0] == '!') {
        is_index = true;
        token++;
      }
      unsigned long long value = strtoull(token, &endptr, 10);
      if (*endptr != '\0') {
        result = -1;
        goto cleanup;
      }

      if (index == 0) {
        result = -1;
        goto cleanup;
      }
      uint8_t node = eval_cells_get(cells, index - 1);
      if (node == EVAL_NATIVE) {
        uint64_t val = EVAL_SET_PAYLOAD(0, value);
        if (is_index) {
          val = EVAL_SET_TAG(val, EVAL_TAG_INDEX);
        }
        uint res = eval_cells_set_word(cells, index - 1, val);
        if (res == (uint)-1) {
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
  size_t index = root;
  while (eval_cells_is_set(cells, index)) {
    uint index_cell = eval_cells_get(cells, index);
    assert(index_cell != ERROR_VALUE);
    printf("%zu|%zu ", index_cell, index);
    index++;
  }
  printf("\n");

  size_t word_index = root;
  while (eval_cells_is_set(cells, word_index)) {
    uint word_index_cell = eval_cells_get(cells, word_index);
    assert(word_index_cell != ERROR_VALUE);
    if (word_index_cell != EVAL_NATIVE) {
      word_index++;
      continue;
    }
    uint word_index_word = eval_cells_get_word(cells, word_index);
    assert(word_index_word != ERROR_VALUE);
    uint8_t tag = EVAL_GET_TAG(word_index_word);
    int64_t payload = EVAL_GET_PAYLOAD(word_index_word);
    printf("(%d)%ld|%zu ", tag, payload, word_index);
    word_index++;
  }
  printf("\n");
}
