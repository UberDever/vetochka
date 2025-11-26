#ifndef __EVAL_NATIVE__
#define __EVAL_NATIVE__

#include "api.h"

#define NATIVE_TYPE_INTEGER 0
#define NATIVE_TYPE_LIST    1

size_t _native_io_print(eval_state_t*, size_t);

#endif
