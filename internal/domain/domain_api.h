#ifndef __INTERNAL_DOMAIN_DOMAIN_API_H__
#define __INTERNAL_DOMAIN_DOMAIN_API_H__

#include <stdint.h>

typedef uint8_t u8;
typedef intptr_t sint;
typedef uintptr_t uint;

#define ERR_VAL -1

_Static_assert(sizeof(void (*)()) <= 8, "Function pointer too large");

#endif
