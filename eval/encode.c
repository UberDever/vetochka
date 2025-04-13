#include "common.h"
#include "stb_ds.h"
#include <stdbool.h>
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
        uint64_t val = SET_PAYLOAD(0, value);
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