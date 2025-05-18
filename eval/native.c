#include <stdio.h>

#include "native.h"
#include "util.h"

size_t _native_io_println(eval_state_t* state, size_t arg) {
  logg("Got %zu", arg);
  return arg;
}
