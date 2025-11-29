#ifndef __INTERNAL_NATIVE_NATIVE_API_H__
#define __INTERNAL_NATIVE_NATIVE_API_H__

#include <stddef.h>

struct eval_state_t;
typedef size_t (*native_function_t)(struct eval_state_t*, size_t);

#endif
