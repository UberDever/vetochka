#ifndef __EVAL_NATIVE__
#define __EVAL_NATIVE__

#include "api.h"

#define WORD_TAG_NUMBER 0
#define WORD_TAG_FUNC   1

#define VALUE_TYPE_NUMBER 0

size_t _native_io_println(eval_state_t*, size_t);

#endif
