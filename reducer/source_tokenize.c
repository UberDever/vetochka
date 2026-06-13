#include "domain_api.h"

#define TOKEN_TYPE_ITEMS(X, P)                                                                     \
  X(P, EOF, 0, "eof")                                                                              \
  X(P, ERROR, 1, "error")                                                                          \
  X(P, STRUCTURED_COMMENT, 2, "structured comment")                                                \
  X(P, NEWLINE, 3, "newline")                                                                      \
  X(P, STRING_LITERAL, 4, "string literal")                                                        \
  X(P, INTEGER_LITERAL, 5, "integer literal")                                                      \
  X(P, DELTA_NODE, 6, "delta node")                                                                \
  X(P, IDENTIFIER, 7, "identifier")                                                                \
  X(P, OPERATOR, 8, "operator")                                                                    \
  X(P, UNARY, 9, "unary")                                                                          \
  X(P, DELIMETER, 10, "delimeter")                                                                 \
  X(P, SEMICOLON, 11, "semicolon")                                                                 \
  X(P, KW_DO, 12, "do")                                                                            \
  X(P, KW_END, 13, "end")

DECL_TYPED_ENUM(token_type_t, u8, TOKEN_TYPE, TOKEN_TYPE_ITEMS)

typedef struct token_t {
  token_type_t type;
  size_t begin, end, line, col;
} token_t;

struct lexer_t {
  span_cbyte_t text;
  size_t pos;
  size_t line;
  size_t col;
  token_t cur;
  token_t prev;
  size_t layout_depth;
  u64 layout_stack;
  bool has_prev;
  bool after_annot_close;
  bool at_eof;
  error_t error;
};

#define LAYOUT_NORMAL_BRACKET 1u
#define LAYOUT_ANNOT_BRACKET  2u
#define LAYOUT_PAREN          3u
#define LAYOUT_STACK_MAX      32u

static bool lexer_is_horizontal_ws(byte c) {
  return c == 0x09 || c == 0x0B || c == 0x0C || c == 0x20;
}

static bool lexer_is_newline(byte c) {
  return c == 0x0A || c == 0x0D;
}

static bool lexer_is_digit(byte c) {
  return c >= '0' && c <= '9';
}

static bool lexer_is_ident_start(byte c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool lexer_is_ident_continue(byte c) {
  if (lexer_is_ident_start(c) || lexer_is_digit(c)) { return true; }
  switch (c) {
    case '?':
    case '=':
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '<':
    case '>':
    case '!':
    case '&':
    case '|': return true;
    default: return false;
  }
}

static bool lexer_is_operator_char(byte c) {
  switch (c) {
    case '=':
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '<':
    case '>':
    case '!':
    case '&':
    case '|':
    case ':': return true;
    default: return false;
  }
}

static bool lexer_is_unary_literal(byte c) {
  switch (c) {
    case '!':
    case '-':
    case '~':
    case '*':
    case '&': return true;
    default: return false;
  }
}

static bool lexer_starts_delta(span_cbyte_t text, size_t pos) {
  return pos + 2 <= text.len && text.data[pos] == 0xCE && text.data[pos + 1] == 0x94;
}

static bool lexer_starts_line_continue(span_cbyte_t text, size_t pos) {
  return pos + 3 <= text.len && text.data[pos] == '.' && text.data[pos + 1] == '.'
         && text.data[pos + 2] == '.';
}

static bool lexer_text_eq(
    span_cbyte_t source, size_t begin, size_t end, const char* text, size_t len) {
  if (end - begin != len) { return false; }
  for (size_t i = 0; i < len; ++i) {
    if (source.data[begin + i] != (byte)text[i]) { return false; }
  }
  return true;
}

static bool lexer_token_text_eq(
    struct lexer_t const* self, token_t token, const char* text, size_t len) {
  return lexer_text_eq(self->text, token.begin, token.end, text, len);
}

static void lexer_advance_byte(struct lexer_t* self) {
  const byte c = self->text.data[self->pos++];
  if (c == '\r' && self->pos < self->text.len && self->text.data[self->pos] == '\n') {
    ++self->pos;
  }
  if (lexer_is_newline(c)) {
    ++self->line;
    self->col = 1;
  } else {
    ++self->col;
  }
}

static token_t lexer_make_token(
    struct lexer_t const* self, token_type_t type, size_t begin, size_t line, size_t col) {
  return CTOR(token_t, .type = type, .begin = begin, .end = self->pos, .line = line, .col = col);
}

static void lexer_set_error(struct lexer_t* self, size_t begin, size_t line, size_t col) {
  self->error = ERROR_GENERIC;
  self->cur = CTOR(
      token_t,
      .type = CTOR(token_type_t, TOKEN_TYPE_ERROR),
      .begin = begin,
      .end = self->pos,
      .line = line,
      .col = col);
}

static bool lexer_skip_line_comment(struct lexer_t* self) {
  if (self->pos + 2 > self->text.len || self->text.data[self->pos] != ';'
      || self->text.data[self->pos + 1] != ';') {
    return false;
  }
  lexer_advance_byte(self);
  lexer_advance_byte(self);
  while (self->pos < self->text.len && !lexer_is_newline(self->text.data[self->pos])) {
    lexer_advance_byte(self);
  }
  return true;
}

static bool lexer_skip_block_comment(struct lexer_t* self) {
  if (self->pos + 2 > self->text.len || self->text.data[self->pos] != '#'
      || self->text.data[self->pos + 1] != '|') {
    return false;
  }

  const size_t begin = self->pos;
  const size_t line = self->line;
  const size_t col = self->col;
  size_t depth = 0;

  while (self->pos < self->text.len) {
    if (self->pos + 2 <= self->text.len && self->text.data[self->pos] == '#'
        && self->text.data[self->pos + 1] == '|') {
      ++depth;
      lexer_advance_byte(self);
      lexer_advance_byte(self);
      continue;
    }
    if (self->pos + 2 <= self->text.len && self->text.data[self->pos] == '|'
        && self->text.data[self->pos + 1] == '#') {
      --depth;
      lexer_advance_byte(self);
      lexer_advance_byte(self);
      if (depth == 0) { return true; }
      continue;
    }
    lexer_advance_byte(self);
  }

  lexer_set_error(self, begin, line, col);
  return false;
}

static bool lexer_skip_line_continue(struct lexer_t* self) {
  if (!lexer_starts_line_continue(self->text, self->pos)) { return false; }

  const size_t save_pos = self->pos;
  const size_t save_line = self->line;
  const size_t save_col = self->col;

  lexer_advance_byte(self);
  lexer_advance_byte(self);
  lexer_advance_byte(self);

  while (self->pos < self->text.len && lexer_is_horizontal_ws(self->text.data[self->pos])) {
    lexer_advance_byte(self);
  }

  (void)lexer_skip_line_comment(self);

  if (self->pos < self->text.len && lexer_is_newline(self->text.data[self->pos])) {
    lexer_advance_byte(self);
    return true;
  }

  self->pos = save_pos;
  self->line = save_line;
  self->col = save_col;
  return false;
}

static bool lexer_skip_trivia(struct lexer_t* self) {
  for (;;) {
    while (self->pos < self->text.len && lexer_is_horizontal_ws(self->text.data[self->pos])) {
      lexer_advance_byte(self);
    }
    if (lexer_skip_line_continue(self)) { continue; }
    if (lexer_skip_line_comment(self)) { continue; }
    if (self->pos + 2 <= self->text.len && self->text.data[self->pos] == '#'
        && self->text.data[self->pos + 1] == '|') {
      if (!lexer_skip_block_comment(self)) { return false; }
      continue;
    }
    return true;
  }
}

static void lexer_next_raw(struct lexer_t* self) {
  if (self->error != ERROR_SUCCESS) { return; }
  if (!lexer_skip_trivia(self)) { return; }

  const size_t begin = self->pos;
  const size_t line = self->line;
  const size_t col = self->col;

  if (self->pos >= self->text.len) {
    self->at_eof = true;
    self->cur = CTOR(
        token_t,
        .type = CTOR(token_type_t, TOKEN_TYPE_EOF),
        .begin = begin,
        .end = begin,
        .line = line,
        .col = col);
    return;
  }

  const byte c = self->text.data[self->pos];

  if (lexer_is_newline(c)) {
    lexer_advance_byte(self);
    self->cur = lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_NEWLINE), begin, line, col);
    return;
  }

  if (self->pos + 2 <= self->text.len && c == '#' && self->text.data[self->pos + 1] == ';') {
    lexer_advance_byte(self);
    lexer_advance_byte(self);
    self->cur =
        lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_STRUCTURED_COMMENT), begin, line, col);
    return;
  }

  if (c == '{') {
    size_t depth = 0;
    do {
      if (self->text.data[self->pos] == '{') { ++depth; }
      if (self->text.data[self->pos] == '}') {
        --depth;
        lexer_advance_byte(self);
        if (depth == 0) {
          self->cur = lexer_make_token(
              self, CTOR(token_type_t, TOKEN_TYPE_STRING_LITERAL), begin, line, col);
          return;
        }
        continue;
      }
      lexer_advance_byte(self);
    } while (self->pos < self->text.len);

    lexer_set_error(self, begin, line, col);
    return;
  }

  if (lexer_is_digit(c)) {
    lexer_advance_byte(self);
    if (c == '0' && self->pos < self->text.len && lexer_is_digit(self->text.data[self->pos])) {
      while (self->pos < self->text.len && lexer_is_digit(self->text.data[self->pos])) {
        lexer_advance_byte(self);
      }
      lexer_set_error(self, begin, line, col);
      return;
    }
    while (self->pos < self->text.len && lexer_is_digit(self->text.data[self->pos])) {
      lexer_advance_byte(self);
    }
    self->cur =
        lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_INTEGER_LITERAL), begin, line, col);
    return;
  }

  if (c == '^') {
    lexer_advance_byte(self);
    self->cur = lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_DELTA_NODE), begin, line, col);
    return;
  }

  if (lexer_starts_delta(self->text, self->pos)) {
    lexer_advance_byte(self);
    lexer_advance_byte(self);
    self->cur = lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_DELTA_NODE), begin, line, col);
    return;
  }

  if (lexer_is_ident_start(c)) {
    lexer_advance_byte(self);
    while (self->pos < self->text.len && lexer_is_ident_continue(self->text.data[self->pos])) {
      lexer_advance_byte(self);
    }

    if (lexer_text_eq(self->text, begin, self->pos, "do", 2)) {
      self->cur = lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_KW_DO), begin, line, col);
      return;
    }
    if (lexer_text_eq(self->text, begin, self->pos, "end", 3)) {
      self->cur = lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_KW_END), begin, line, col);
      return;
    }

    self->cur = lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_IDENTIFIER), begin, line, col);
    return;
  }

  switch (c) {
    case '@':
      lexer_advance_byte(self);
      if (self->pos < self->text.len && self->text.data[self->pos] == '[') {
        lexer_advance_byte(self);
      }
      self->cur =
          lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), begin, line, col);
      return;
    case '[':
    case ']':
    case '(':
    case ')':
    case '.':
    case ',':
      lexer_advance_byte(self);
      self->cur =
          lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), begin, line, col);
      return;
    case ';':
      lexer_advance_byte(self);
      self->cur =
          lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_SEMICOLON), begin, line, col);
      return;
    case '~':
      lexer_advance_byte(self);
      self->cur = lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_UNARY), begin, line, col);
      return;
    default: break;
  }

  if (lexer_is_operator_char(c)) {
    lexer_advance_byte(self);
    while (self->pos < self->text.len && lexer_is_operator_char(self->text.data[self->pos])) {
      lexer_advance_byte(self);
    }
    if (self->pos == begin + 1 && c == ':') {
      self->cur =
          lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), begin, line, col);
    } else if (self->pos == begin + 1 && lexer_is_unary_literal(c)) {
      self->cur = lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_UNARY), begin, line, col);
    } else {
      self->cur = lexer_make_token(self, CTOR(token_type_t, TOKEN_TYPE_OPERATOR), begin, line, col);
    }
    return;
  }

  lexer_advance_byte(self);
  lexer_set_error(self, begin, line, col);
}

static bool lexer_token_can_end_expression(struct lexer_t const* self, token_t token) {
  switch (token.type.value) {
    case TOKEN_TYPE_STRING_LITERAL:
    case TOKEN_TYPE_INTEGER_LITERAL:
    case TOKEN_TYPE_DELTA_NODE:
    case TOKEN_TYPE_IDENTIFIER:
    case TOKEN_TYPE_KW_END: return true;
    case TOKEN_TYPE_DELIMETER:
      return lexer_token_text_eq(self, token, ")", 1) || lexer_token_text_eq(self, token, "]", 1);
    default: return false;
  }
}

static void lexer_layout_push(struct lexer_t* self, u64 kind) {
  if (self->layout_depth >= LAYOUT_STACK_MAX) {
    self->error = ERROR_OVERFLOW;
    self->cur = CTOR(
        token_t,
        .type = CTOR(token_type_t, TOKEN_TYPE_ERROR),
        .begin = self->cur.begin,
        .end = self->cur.end,
        .line = self->cur.line,
        .col = self->cur.col);
    return;
  }
  self->layout_stack |= kind << (self->layout_depth * 2U);
  ++self->layout_depth;
}

static u64 lexer_layout_pop(struct lexer_t* self) {
  if (self->layout_depth == 0) { return 0; }
  --self->layout_depth;
  const u64 kind = (self->layout_stack >> (self->layout_depth * 2U)) & 3U;
  self->layout_stack &= ~((u64)3U << (self->layout_depth * 2U));
  return kind;
}

static void lexer_layout_note_token(struct lexer_t* self) {
  self->after_annot_close = false;
  if (self->cur.type.value == TOKEN_TYPE_DELIMETER) {
    if (lexer_token_text_eq(self, self->cur, "@[", 2)) {
      lexer_layout_push(self, LAYOUT_ANNOT_BRACKET);
    } else if (lexer_token_text_eq(self, self->cur, "[", 1)) {
      lexer_layout_push(self, LAYOUT_NORMAL_BRACKET);
    } else if (lexer_token_text_eq(self, self->cur, "(", 1)) {
      lexer_layout_push(self, LAYOUT_PAREN);
    } else if (lexer_token_text_eq(self, self->cur, "]", 1)) {
      self->after_annot_close = lexer_layout_pop(self) == LAYOUT_ANNOT_BRACKET;
    } else if (lexer_token_text_eq(self, self->cur, ")", 1)) {
      (void)lexer_layout_pop(self);
    }
  }
  if (self->error != ERROR_SUCCESS) { return; }
  self->prev = self->cur;
  self->has_prev = true;
}

MUH_PRIVATE void lexer_next(struct lexer_t* self) {
  if (self->line == 0) {
    self->line = 1;
    self->col = 1;
  }

  for (;;) {
    lexer_next_raw(self);
    if (self->error != ERROR_SUCCESS || self->cur.type.value == TOKEN_TYPE_EOF) { return; }

    if (self->cur.type.value != TOKEN_TYPE_NEWLINE) {
      lexer_layout_note_token(self);
      return;
    }

    if (self->layout_depth == 0 && self->has_prev && !self->after_annot_close
        && lexer_token_can_end_expression(self, self->prev)) {
      self->cur = CTOR(
          token_t,
          .type = CTOR(token_type_t, TOKEN_TYPE_SEMICOLON),
          .begin = self->cur.begin,
          .end = self->cur.end,
          .line = self->cur.line,
          .col = self->cur.col);
      self->prev = self->cur;
      self->has_prev = true;
      self->after_annot_close = false;
      return;
    }

    self->after_annot_close = false;
  }
}
