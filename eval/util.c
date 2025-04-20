#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "common.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

static const char base64_table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * base64_encode() - base64-encode some binary data
 * @src: the binary data to encode
 * @srclen: the length of @src in bytes
 * @dst: (output) the base64-encoded string.  Not NUL-terminated.
 *
 * Encodes data using base64 encoding, i.e. the "Base 64 Encoding" specified
 * by RFC 4648, including the  '='-padding.
 *
 * Return: the length of the resulting base64-encoded string in bytes.
 */
int _base64_encode(const u8* src, int srclen, char* dst) {
  u32 ac = 0;
  int bits = 0;
  int i;
  char* cp = dst;

  for (i = 0; i < srclen; i++) {
    ac = (ac << 8) | src[i];
    bits += 8;
    do {
      bits -= 6;
      *cp++ = base64_table[(ac >> bits) & 0x3f];
    } while (bits >= 6);
  }
  if (bits) {
    *cp++ = base64_table[(ac << (6 - bits)) & 0x3f];
    bits -= 6;
  }
  while (bits < 0) {
    *cp++ = '=';
    bits += 2;
  }
  return cp - dst;
}

/**
 * base64_decode() - base64-decode a string
 * @src: the string to decode.  Doesn't need to be NUL-terminated.
 * @srclen: the length of @src in bytes
 * @dst: (output) the decoded binary data
 *
 * Decodes a string using base64 encoding, i.e. the "Base 64 Encoding"
 * specified by RFC 4648, including the  '='-padding.
 *
 * This implementation hasn't been optimized for performance.
 *
 * Return: the length of the resulting decoded binary data in bytes,
 *	   or -1 if the string isn't a valid base64 string.
 */
int _base64_decode(const char* src, int srclen, u8* dst) {
  u32 ac = 0;
  int bits = 0;
  int i;
  u8* bp = dst;

  for (i = 0; i < srclen; i++) {
    const char* p = strchr(base64_table, src[i]);

    if (src[i] == '=') {
      ac = (ac << 6);
      bits += 6;
      if (bits >= 8)
        bits -= 8;
      continue;
    }
    if (p == NULL || src[i] == 0)
      return -1;
    ac = (ac << 6) | (p - base64_table);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      *bp++ = (u8)(ac >> bits);
    }
  }
  if (ac & ((1 << bits) - 1))
    return -1;
  return bp - dst;
}

// Initialize an empty buffer with a small initial capacity
void _sb_init(StringBuffer s) {
  s->len = 0;
  s->cap = 64; // or any small power of two
  s->buf = malloc(s->cap);
  if (!s->buf) {
    assert(false);
  }
  s->buf[0] = '\0';
}

// Free the buffer when done
void _sb_free(StringBuffer s) {
  free(s->buf);
  s->buf = NULL;
  s->len = s->cap = 0;
}

// Ensure there's room for at least `needed` more bytes (excluding NUL)
static void sb_ensure(StringBuffer s, size_t needed) {
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
void _sb_append_data(StringBuffer s, const char* data, size_t n) {
  sb_ensure(s, n);
  memcpy(s->buf + s->len, data, n);
  s->len += n;
  s->buf[s->len] = '\0';
}

// Append a C‑string
void _sb_append_str(StringBuffer s, const char* str) {
  _sb_append_data(s, str, strlen(str));
}

// Append a single character
void _sb_append_char(StringBuffer s, char c) {
  sb_ensure(s, 1);
  s->buf[s->len++] = c;
  s->buf[s->len] = '\0';
}

// Append formatted text like printf()
void _sb_printf(StringBuffer s, const char* fmt, ...) {
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

const char* _sb_str_view(struct StringBuf s) {
  return s.buf;
}

char* _sb_detach(StringBuffer s) {
  char* ret = s->buf; // grab the pointer
  s->buf = NULL;      // reset struct to empty state
  s->len = 0;
  s->cap = 0;
  return ret; // caller must free(ret) when done
}

// Remove the suffix from s if s->buf ends with 'suffix'.
// Returns 1 if the suffix was found and removed, 0 otherwise.
int _sb_try_chop_suffix(StringBuffer s, const char* suffix) {
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
