#ifndef __EVAL_COMMON__
#define __EVAL_COMMON__

#include <stdbool.h>
#include <stddef.h>

#include "api.h"

#define ERR_VAL -1

#define logg(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);
#define logg_s(s)      printf("[%s:%d] " s "\n", __FILE__, __LINE__);

#define BITS_PER_CELL    2
#define CELLS_PER_WORD   (BITS_PER_WORD / BITS_PER_CELL)
#define BITS_PER_WORD    (sizeof(u64) * 8)
#define BITMAP_SIZE(cap) (((cap) + BITS_PER_WORD - 1) / BITS_PER_WORD)
u8 _bitmap_get_bit(const u64* bitmap, size_t index);
void _bitmap_set_bit(u64* bitmap, size_t index, u8 value);

int _base64_encode(const u8* src, int srclen, char* dst);
int _base64_decode(const char* src, int srclen, u8* dst);

typedef struct string_buffer_t {
  char* buf;  // Pointer to allocated data (NUL‑terminated)
  size_t len; // Number of bytes currently used, excluding final NUL
  size_t cap; // Total bytes allocated for buf (including space for NUL)
} string_buffer_t;

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

void _errbuf_clear();
void _errbuf_write(const char* format, ...);

#define GET_CELL(cells, index)                                                                     \
  sint index##_cell = eval_cells_get(cells, index);                                                \
  EXPECT(index##_cell != ERR_VAL, ERROR_INVALID_CELL, "");

#define GET_WORD(cells, index)                                                                     \
  sint index##_word = eval_cells_get_word(cells, index);                                           \
  EXPECT(index##_cell != ERR_VAL, ERROR_INVALID_WORD, "");

#define DEREF(cells, index)                                                                        \
  if (index##_cell == EVAL_REF) {                                                                  \
    GET_WORD(cells, index);                                                                        \
    index += (sint)index##_word;                                                                   \
    size_t new_index = index;                                                                      \
    GET_CELL(cells, new_index)                                                                     \
    EXPECT(new_index_cell != EVAL_REF, ERROR_REF_TO_REF, "");                                      \
  }

static inline u8 eval_tv_get_tag(u64 tagged_value) {
  return (u8)(tagged_value & 0xF);
}

static inline i64 eval_tv_get_payload_signed(u64 tagged_value) {
  return (i64)(tagged_value & ~0xFULL) >> 4;
}

static inline u64 eval_tv_get_payload_unsigned(u64 tagged_value) {
  return tagged_value >> 4;
}

static inline u64 eval_tv_set_tag(u64 tagged_value, u8 new_tag) {
  return (tagged_value & ~0xFULL) | (new_tag & 0xF);
}

static inline u64 eval_tv_set_payload_signed(u64 tagged_value, i64 new_payload) {
  u64 tag_bits = tagged_value & 0xF;
  return ((u64)new_payload << 4) | tag_bits;
}

static inline u64 eval_tv_set_payload_unsigned(u64 tagged_value, u64 new_payload) {
  u64 tag_bits = tagged_value & 0xF;
  return (new_payload << 4) | tag_bits;
}

static inline u64 eval_tv_new_tagged_value_signed(u8 tag, i64 payload) {
  return ((u64)payload << 4) | (tag & 0xF);
}

static inline u64 eval_tv_new_tagged_value_unsigned(u8 tag, u64 payload) {
  return (payload << 4) | (tag & 0xF);
}

#define EVAL_NIL  0
#define EVAL_TREE 1
// #define EVAL_APPLY 2
#define EVAL_REF 3

#define EVAL_TAG_NUMBER 0
#define EVAL_TAG_INDEX  1
#define EVAL_TAG_FUNC   2

typedef enum stack_entry_t {
  STACK_ENTRY_INVALID,
  STACK_ENTRY_INDEX,
  STACK_ENTRY_CALCULATED
} stack_entry_t;

typedef enum calculated_index_t {
  CALCULATED_INDEX_INVALID,
  CALCULATED_INDEX_RULE2,
  CALCULATED_INDEX_RULE3C
} calculated_index_t;

struct StackEntry {
  stack_entry_t tag;

  union {
    size_t as_index;
    calculated_index_t as_calculated;
  };
};

struct json_parser_t;

struct EvalState_impl {
  Allocator cells;
  Stack control_stack;
  size_t* value_stack;
  u64* free_bitmap;
  size_t free_capacity;

  uint8_t error_code;
  const char* error;
};

void _eval_control_stack_push_index(EvalState state, size_t index);
void _eval_control_stack_push_calculated(EvalState state, u8 type);
void _eval_value_stack_push(EvalState state, size_t value);
sint _eval_load_json(struct json_parser_t* parser, EvalState state);
sint _eval_cells_load_json(struct json_parser_t* parser, Allocator cells);

typedef struct {
  size_t key;
  size_t value;
} cell_word;

struct Allocator_impl {
  u64* cells;
  u64* cells_bitmap;
  size_t cells_capacity;

  cell_word* payload_index;

  i64* payloads;
};

sint _eval_cells_dump_json(string_buffer_t* json_out, Allocator cells);

enum json_token_t {
  JSON_TOKEN_NULL,
  JSON_TOKEN_BOOL,
  JSON_TOKEN_NUMBER,
  JSON_TOKEN_STRING,
  JSON_TOKEN_ARRAY,
  JSON_TOKEN_OBJECT,
};

struct jsmntok;

typedef struct json_parser_t {
  const char* json;
  struct jsmntok* tokens;
  size_t tokens_len;
  size_t cur_token;
  bool was_err;
  bool at_eof;

  size_t entries_count;

  enum {
    JSON_DIGESTED_INVALID,
    JSON_DIGESTED_NULL,
    JSON_DIGESTED_BOOL,
    JSON_DIGESTED_NUMBER,
    JSON_DIGESTED_STRING,
    JSON_DIGESTED_ARRAY,
    JSON_DIGESTED_OBJECT,
  } digested;

  string_buffer_t digested_string;

  union {
    double digested_number;
    bool digested_bool;
  };
} json_parser_t;

// NOTE: doesn't take ownership of json
sint _json_parser_init(const char* json, json_parser_t* parser);
void _json_parser_free(json_parser_t* parser);
bool _json_parser_match(json_parser_t* parser, enum json_token_t token);
bool _json_parser_eat(json_parser_t* parser, enum json_token_t token);
const char* _json_parser_get_string(json_parser_t* parser);

#define _JSON_PARSER_EAT(type, errval)                                                             \
  _json_parser_eat(parser, JSON_TOKEN_##type);                                                     \
  if (parser->was_err) {                                                                           \
    err = errval;                                                                                  \
    goto cleanup;                                                                                  \
  }                                                                                                \
  if (parser->at_eof) {                                                                            \
    goto cleanup;                                                                                  \
  }

#define _JSON_PARSER_EAT_KEY(key, errval)                                                          \
  _JSON_PARSER_EAT(STRING, 1);                                                                     \
  if (strcmp(_json_parser_get_string(parser), key) != 0) {                                         \
    err = errval;                                                                                  \
    logg_s("expected " key " field");                                                              \
    goto cleanup;                                                                                  \
  }

#endif // __EVAL_COMMON__
