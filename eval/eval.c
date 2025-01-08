#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int8_t i8;
typedef uint64_t u64;

#define ERROR_BUF_SIZE 1024
static char g_error_buf[ERROR_BUF_SIZE] = {};

#define NODE_SIZE 64
#define TAG_SIZE 2
#define DATA_SIZE ((NODE_SIZE - TAG_SIZE) / 2)
#define DATA_MAX ~(1 << DATA_SIZE)

struct EvalState {
  u64 root;
  u64 *nodes;
  u64 nodes_size;

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

void eval(struct EvalState *state) {
  state->error_code = 1;
  sprintf(g_error_buf, "Hey there!");
  state->error = g_error_buf;
}
