#ifndef __INTERNAL_UTILITY_STRING_BUFFER_API_H__
#define __INTERNAL_UTILITY_STRING_BUFFER_API_H__

#include <stddef.h>

typedef struct string_buffer_t {
  char* buf;
  size_t len;
  size_t cap;
} string_buffer_t;

void _sb_init(string_buffer_t* s);
void _sb_free(string_buffer_t* s);
void _sb_clear(string_buffer_t* s);
void _sb_append_data(string_buffer_t* s, const char* data, size_t n);
void _sb_append_str(string_buffer_t* s, const char* str);
void _sb_append_char(string_buffer_t* s, char c);
void _sb_printf(string_buffer_t* s, const char* fmt, ...);
const char* _sb_str_view(string_buffer_t* s);
char* _sb_detach(string_buffer_t* s);
int _sb_try_chop_suffix(string_buffer_t* s, const char* suffix);

#endif
