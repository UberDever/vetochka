#ifndef __EVAL_UTIL__
#define __EVAL_UTIL__

#include <stdbool.h>
#include <stddef.h>

#include "api.h"

#define logg(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);
#define logg_s(s)      printf("[%s:%d] " s "\n", __FILE__, __LINE__);

#define CHECK_ERROR(on_error)                                                                      \
  if (err) {                                                                                       \
    on_error;                                                                                      \
    goto error;                                                                                    \
  }

struct string_buffer_t {
  char* buf;
  size_t len;
  size_t cap;
};

void _sb_init(struct string_buffer_t* s);
void _sb_free(struct string_buffer_t* s);
void _sb_clear(struct string_buffer_t* s);
void _sb_append_data(struct string_buffer_t* s, const char* data, size_t n);
void _sb_append_str(struct string_buffer_t* s, const char* str);
void _sb_append_char(struct string_buffer_t* s, char c);
void _sb_printf(struct string_buffer_t* s, const char* fmt, ...);
const char* _sb_str_view(struct string_buffer_t* s);
char* _sb_detach(struct string_buffer_t* s);
int _sb_try_chop_suffix(struct string_buffer_t* s, const char* suffix);

u8 _bitmap_get_bit(const uint* bitmap, size_t index);
void _bitmap_set_bit(uint* bitmap, size_t index, u8 value);

#endif // __EVAL_UTIL__
