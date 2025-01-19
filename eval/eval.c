#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

#define ERROR_BUF_SIZE 1024
static char g_error_buf[ERROR_BUF_SIZE] = {};

struct EvalState {
  uint root;
  uint *nodes;
  uint nodes_size;

  i8 error_code;
  char *error;
};

void reset(struct EvalState *state) {
  state->root = 0;
  state->nodes = NULL;
  state->nodes_size = 0;
  state->error_code = 0;
  state->error = NULL;
}

uint eval(struct EvalState *state) {
  /* state->error_code = 1; */
  /* sprintf(g_error_buf, "size: %zu\n", state->nodes_size); */
  /* for (uint i = 0; i < state->nodes_size; ++i) { */
  /*   sprintf(g_error_buf, "%s%zu ", g_error_buf, state->nodes[i]); */
  /* } */
  /* state->error = g_error_buf; */
  return 1;
}
