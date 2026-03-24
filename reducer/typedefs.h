#ifndef __REDUCER_TYPEDEFS_H__
#define __REDUCER_TYPEDEFS_H__

#include <stdbool.h>
#include <stddef.h>
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

#define OPT_T(T)                                                               \
  struct opt_##T {                                                             \
    bool has_value;                                                            \
    T value;                                                                   \
  }

OPT_T(size_t);

#define PAIR_T(T1, T2)                                                         \
  struct pair_##T1##_##T2 {                                                    \
    T1 first;                                                                  \
    T2 second;                                                                 \
  }

PAIR_T(size_t, size_t);

typedef struct span_byte_t {
  byte *data;
  u64 len;
} span_byte_t;

typedef struct span_cbyte_t {
  const byte *data;
  u64 len;
} span_cbyte_t;

typedef i32 error_t;

#define ERROR_SUCCESS 0
#define ERROR_GENERIC -1
#define ERROR_INVALID_PARAM -2
#define ERROR_OUT_OF_BOUNDS -3
#define ERROR_OVERFLOW -4
#define ERROR_NOMEM -5
#define ERROR_INTERNAL -6

#define my_debug(fmt, ...)                                                     \
  fprintf(stderr, "[DEBUG] %s %d " fmt "\n", __FILE_NAME__, __LINE__,          \
          __VA_ARGS__)

#endif // __REDUCER_TYPEDEFS_H__
