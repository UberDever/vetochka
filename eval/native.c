#include "native.h"
#include "common.h"
#include <stdio.h>

size_t _native_io_println(EvalState state, size_t arg) {
  logg("Got %zu", arg);
  return arg;
}
