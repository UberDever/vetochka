#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#define STB_DS_IMPLEMENTATION
#include "vendor/stb_ds.h"

// NOTE: this is an implementation
#include "vendor/jsmn.h"

#include "util.h"

// Initialize an empty buffer with a small initial capacity
void _sb_init(struct string_buffer_t* s) {
  s->len = 0;
  s->cap = 64; // or any small power of two
  s->buf = malloc(s->cap);
  if (!s->buf) {
    assert(false);
  }
  s->buf[0] = '\0';
}

// Free the buffer when done
void _sb_free(struct string_buffer_t* s) {
  free(s->buf);
  s->buf = NULL;
  s->len = s->cap = 0;
}

void _sb_clear(struct string_buffer_t* s) {
  s->buf[0] = '\0';
  s->len = 0;
}

// Ensure there's room for at least `needed` more bytes (excluding NUL)
static void sb_ensure(struct string_buffer_t* s, size_t needed) {
  if (s->len + needed + 1 > s->cap) {
    size_t newcap = s->cap;
    while (newcap < s->len + needed + 1) {
      newcap *= 2;
    }
    char* newbuf = realloc(s->buf, newcap);
    if (!newbuf) {
      assert(false);
    }
    s->buf = newbuf;
    s->cap = newcap;
  }
}

// Append a raw block of data (not necessarily NUL‑terminated)
void _sb_append_data(struct string_buffer_t* s, const char* data, size_t n) {
  sb_ensure(s, n);
  memcpy(s->buf + s->len, data, n);
  s->len += n;
  s->buf[s->len] = '\0';
}

// Append a C‑string
void _sb_append_str(struct string_buffer_t* s, const char* str) {
  _sb_append_data(s, str, strlen(str));
}

// Append a single character
void _sb_append_char(struct string_buffer_t* s, char c) {
  sb_ensure(s, 1);
  s->buf[s->len++] = c;
  s->buf[s->len] = '\0';
}

// Append formatted text like printf()
void _sb_printf(struct string_buffer_t* s, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  // Determine required length
  va_list ap2;
  va_copy(ap2, ap);
  int needed = vsnprintf(NULL, 0, fmt, ap2);
  va_end(ap2);

  if (needed > 0) {
    sb_ensure(s, needed);
    vsnprintf(s->buf + s->len, needed + 1, fmt, ap);
    s->len += needed;
  }

  va_end(ap);
}

const char* _sb_str_view(struct string_buffer_t* s) {
  return s->buf;
}

char* _sb_detach(struct string_buffer_t* s) {
  char* ret = s->buf; // grab the pointer
  s->buf = NULL;      // reset struct to empty state
  s->len = 0;
  s->cap = 0;
  return ret; // caller must free(ret) when done
}

// Remove the suffix from s if s->buf ends with 'suffix'.
// Returns 1 if the suffix was found and removed, 0 otherwise.
int _sb_try_chop_suffix(struct string_buffer_t* s, const char* suffix) {
  size_t slen = strlen(suffix);
  if (s->len < slen) {
    return 0; // Buffer shorter than suffix => no match
  }

  // Compare the tail of s->buf against suffix
  if (memcmp(s->buf + s->len - slen, suffix, slen) == 0) {
    s->len -= slen;        // Remove suffix length
    s->buf[s->len] = '\0'; // Re‐terminate the string
    return 1;              // Suffix was removed
  }
  return 0; // No match, unchanged
}
