#ifndef REDUCER_DOMAIN_API_H
#define REDUCER_DOMAIN_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#if defined(MUH_BUILDING)
#define MUH_PUBLIC __declspec(dllexport)
#else
#define MUH_PUBLIC __declspec(dllimport)
#endif
#define MUH_PRIVATE
#define MUH_PRINTF_LIKE(fmt_index, first_arg)
#define MUH_UNUSED
#define MUH_NORETURN __declspec(noreturn)
#define MUH_COLD
#define MUH_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define MUH_PUBLIC                            __attribute__((visibility("default")))
#define MUH_PRIVATE                           __attribute__((visibility("hidden")))
#define MUH_PRINTF_LIKE(fmt_index, first_arg) __attribute__((format(printf, fmt_index, first_arg)))
#define MUH_UNUSED                            __attribute__((unused))
#define MUH_NORETURN                          __attribute__((noreturn))
#define MUH_COLD                              __attribute__((cold))
#define MUH_NOINLINE                          __attribute__((noinline))
#else
#define MUH_PUBLIC
#define MUH_PRIVATE
#define MUH_PRINTF_LIKE(fmt_index, first_arg)
#define MUH_UNUSED
#define MUH_NORETURN
#define MUH_COLD
#define MUH_NOINLINE
#endif

typedef unsigned char byte;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uintptr_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef intptr_t i64;

#define OPT_T(T)                                                                                   \
  struct opt_##T {                                                                                 \
    bool has_value;                                                                                \
    T value;                                                                                       \
  }

OPT_T(size_t);

#define PAIR_T(T1, T2)                                                                             \
  struct pair_##T1##_##T2 {                                                                        \
    T1 first;                                                                                      \
    T2 second;                                                                                     \
  }

PAIR_T(size_t, size_t);

#define CTOR(T, ...)                                                                               \
  (T) {                                                                                            \
    __VA_ARGS__                                                                                    \
  }

typedef struct span_byte_t {
  byte* data;
  u64 len;
} span_byte_t;

typedef struct span_cbyte_t {
  const byte* data;
  u64 len;
} span_cbyte_t;

typedef i32 error_t;

typedef void* opaque_t;

struct allocator_t;
struct da_byte_t;

#define ERROR_SUCCESS       0
#define ERROR_GENERIC       -1
#define ERROR_INVALID_PARAM -2
#define ERROR_OUT_OF_BOUNDS -3
#define ERROR_OVERFLOW      -4
#define ERROR_NOMEM         -5
#define ERROR_INTERNAL      -6

MUH_PUBLIC void domain_debug(const char* file, int line, const char* fmt, ...)
    MUH_PRINTF_LIKE(3, 4);

#define MUH_DEBUG(...) domain_debug(__FILE__, __LINE__, __VA_ARGS__)

MUH_PUBLIC error_t
domain_da_byte_init(struct da_byte_t* array, const struct allocator_t* allocator);
MUH_PUBLIC void domain_da_byte_free(struct da_byte_t* array);
MUH_PUBLIC span_cbyte_t domain_da_byte_get_span(const struct da_byte_t* array);

/* ---------- generic typed-enum generator ---------- */

#define TENUM_ENUM_CONST_(PREFIX, NAME, VALUE, STR) PREFIX##_##NAME = VALUE,

#define TENUM_VALID_CASE_(PREFIX, NAME, VALUE, STR) case PREFIX##_##NAME:

#define TENUM_STR_CASE_(PREFIX, NAME, VALUE, STR)                                                  \
  case PREFIX##_##NAME: return STR;

#define DECL_TYPED_ENUM(TYPE, STORAGE, PREFIX, ITEMS)                                              \
  typedef struct {                                                                                 \
    STORAGE value;                                                                                 \
  } TYPE;                                                                                          \
                                                                                                   \
  enum { ITEMS(TENUM_ENUM_CONST_, PREFIX) PREFIX##_COUNT };                                        \
                                                                                                   \
  MUH_UNUSED static inline bool TYPE##_is_valid_raw(STORAGE value) {                               \
    switch (value) {                                                                               \
      ITEMS(TENUM_VALID_CASE_, PREFIX)                                                             \
      return true;                                                                                 \
      default: return false;                                                                       \
    }                                                                                              \
  }                                                                                                \
                                                                                                   \
  MUH_UNUSED static inline const char* TYPE##_str(TYPE x) {                                        \
    switch (x.value) {                                                                             \
      ITEMS(TENUM_STR_CASE_, PREFIX)                                                               \
      default: return "<invalid>";                                                                 \
    }                                                                                              \
  }

#endif // REDUCER_DOMAIN_API_H
