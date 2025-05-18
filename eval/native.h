#ifndef __EVAL_NATIVE__
#define __EVAL_NATIVE__

#include "api.h"

#define NATIVE_TAG_NUMBER 0
#define NATIVE_TAG_FUNC   1

size_t _native_io_println(eval_state_t*, size_t);

#endif
