#ifndef __INTERNAL_NATIVE_NATIVE_IMPL_H__
#define __INTERNAL_NATIVE_NATIVE_IMPL_H__

#include "internal/eval/eval_api.h"
#include <stddef.h>
#define NATIVE_TYPE_INTEGER 0
#define NATIVE_TYPE_LIST    1

struct eval_state_t;
size_t _native_io_print(eval_state_t*, size_t);

#endif
