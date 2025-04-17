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
      bool is_index = false;
      if (token[0] == '!') {
        is_index = true;
        token++;
      }
      sint value = strtoull(token, &endptr, 10);
      if (*endptr != '\0') {
        result = -1;
        goto cleanup;
      }

      if (index == 0) {
        result = -1;
        goto cleanup;
      }
      sint node = eval_cells_get(cells, index - 1);
      if (node == EVAL_NATIVE) {
        u8 tag = 0;
        if (is_index) {
          tag = EVAL_TAG_INDEX;
        }
        u64 val = eval_tv_new_tagged_value_signed(tag, value);
        sint res = eval_cells_set_word(cells, index - 1, val);
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
  printf("nodes: ");
  while (eval_cells_is_set(cells, index)) {
    sint index_cell = eval_cells_get(cells, index);
    assert(index_cell != ERR_VAL);
    printf("%c[%zu] ", nodes[index_cell], index);
    index++;
  }
  printf("\n");

  printf("words: ");
  size_t word_index = root;
  while (eval_cells_is_set(cells, word_index)) {
    sint word_index_cell = eval_cells_get(cells, word_index);
    assert(word_index_cell != ERR_VAL);
    if (word_index_cell != EVAL_NATIVE) {
      word_index++;
      continue;
    }
    sint word_index_word = eval_cells_get_word(cells, word_index);
    assert(word_index_word != ERR_VAL);
    u8 tag = eval_tv_get_tag(word_index_word);
    i64 payload = eval_tv_get_payload_signed(word_index_word);
    printf("(%d)%ld[%zu] ", tag, payload, word_index);
    word_index++;
  }
  printf("\n");
}
