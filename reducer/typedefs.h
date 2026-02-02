#ifndef __REDUCER_TYPEDEFS_H__
#define __REDUCER_TYPEDEFS_H__

#include <stdint.h>

typedef unsigned char byte;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uintptr_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef intptr_t i64;

typedef struct span_byte_t {
    byte* data;
    u64 len;
} span_byte_t;

#endif // __REDUCER_TYPEDEFS_H__
