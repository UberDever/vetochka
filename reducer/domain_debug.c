#include "domain_api.h"

#include <stdarg.h>
#include <stdio.h>

void domain_debug(const char* file, int line, const char* fmt, ...) {
  va_list args;

  fprintf(stderr, "[DEBUG] %s:%d ", file, line);

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  fputc('\n', stderr);
}
