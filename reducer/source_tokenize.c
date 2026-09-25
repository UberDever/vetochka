#include "source_impl.h"

// Lexer for docs/new_spec/02_syntax.md: "Source text and trivia", "Tokens" and
// "Automatic semicolon insertion".

#define LAYOUT_BRACKET 1u
#define LAYOUT_ANNOT   2u
#define LAYOUT_PAREN   3u
#define LAYOUT_BLOCK   4u

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
    case '\'':
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '<':
    case '>':
    case '!':
    case '&': return true;
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
    case ':':
    case '^': return true;
    default: return false;
  }
}

static bool lexer_at(struct lexer_t const* self, size_t pos, const char* text) {
  for (size_t i = 0; text[i] != '\0'; ++i) {
    if (pos + i >= self->text.len || self->text.data[pos + i] != (byte)text[i]) { return false; }
  }
  return true;
}

static bool lexer_starts_line_continue(struct lexer_t const* self, size_t pos) {
  return lexer_at(self, pos, "...");
}

// True when the byte at `pos` begins trivia (or the text ends), so nothing is glued to it.
static bool lexer_starts_trivia(struct lexer_t const* self, size_t pos) {
  if (pos >= self->text.len) { return true; }
  const byte c = self->text.data[pos];
  return lexer_is_horizontal_ws(c) || lexer_is_newline(c) || lexer_at(self, pos, ";;")
         || lexer_at(self, pos, "#|") || lexer_at(self, pos, "#;")
         || lexer_starts_line_continue(self, pos);
}

static bool lexer_text_eq(
    span_cbyte_t source, size_t begin, size_t end, const char* text, size_t len) {
  if (end - begin != len) { return false; }
  for (size_t i = 0; i < len; ++i) {
    if (source.data[begin + i] != (byte)text[i]) { return false; }
  }
  return true;
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

static void lexer_emit(struct lexer_t* self, u8 type, size_t begin, size_t line, size_t col) {
  self->cur = lexer_make_token(self, CTOR(token_type_t, type), begin, line, col);
}

static void lexer_set_error(struct lexer_t* self, size_t begin, size_t line, size_t col) {
  self->error = ERROR_GENERIC;
  lexer_emit(self, TOKEN_TYPE_ERROR, begin, line, col);
}

static bool lexer_skip_line_comment(struct lexer_t* self) {
  if (!lexer_at(self, self->pos, ";;")) { return false; }
  while (self->pos < self->text.len && !lexer_is_newline(self->text.data[self->pos])) {
    lexer_advance_byte(self);
  }
  return true;
}

static bool lexer_skip_block_comment(struct lexer_t* self) {
  const size_t begin = self->pos;
  const size_t line = self->line;
  const size_t col = self->col;
  size_t depth = 0;

  while (self->pos < self->text.len) {
    if (lexer_at(self, self->pos, "#|")) {
      ++depth;
      lexer_advance_byte(self);
      lexer_advance_byte(self);
      continue;
    }
    if (lexer_at(self, self->pos, "|#")) {
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

// line_continue ::= "..." t_hws* line_comment? t_nl
static bool lexer_skip_line_continue(struct lexer_t* self) {
  if (!lexer_starts_line_continue(self, self->pos)) { return false; }

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
    if (lexer_at(self, self->pos, "#|")) {
      if (!lexer_skip_block_comment(self)) { return false; }
      continue;
    }
    return true;
  }
}

// A token starting at `begin` is glued when it touches an expression-ending token.
static bool lexer_glued(struct lexer_t const* self, size_t begin) {
  return self->has_prev && self->last_ends_expr && self->last_end == begin;
}

// `do` / `end` directly followed by a lone `:` are label words, not block words.
static bool lexer_label_word_follows(struct lexer_t const* self) {
  return self->pos < self->text.len && self->text.data[self->pos] == ':'
         && (self->pos + 1 >= self->text.len
             || !lexer_is_operator_char(self->text.data[self->pos + 1]));
}

static void lexer_lex_operator_run(struct lexer_t* self, size_t begin, size_t line, size_t col) {
  const bool glued_left = self->has_prev && self->last_end == begin;
  while (self->pos < self->text.len && lexer_is_operator_char(self->text.data[self->pos])) {
    lexer_advance_byte(self);
  }
  const bool lone_colon = self->pos == begin + 1 && self->text.data[begin] == ':';

  if (lone_colon) {
    // A lone `:` is punctuation: it must be stuck to a label word.
    if (glued_left && self->last_is_label_word) {
      lexer_emit(self, TOKEN_TYPE_G_COLON, begin, line, col);
    } else {
      lexer_set_error(self, begin, line, col);
    }
    return;
  }

  // 1. glued to a preceding identifier, literal, `)` or `]`: syntax error.
  if (glued_left && self->last_ends_expr && self->prev.type.value != TOKEN_TYPE_KW_END) {
    lexer_set_error(self, begin, line, col);
    return;
  }
  // 2. glued to its right neighbor: op_prefix; 3. otherwise op_infix.
  if (!lexer_starts_trivia(self, self->pos)) {
    lexer_emit(self, TOKEN_TYPE_OP_PREFIX, begin, line, col);
  } else {
    lexer_emit(self, TOKEN_TYPE_OP_INFIX, begin, line, col);
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
    lexer_emit(self, TOKEN_TYPE_EOF, begin, line, col);
    return;
  }

  const byte c = self->text.data[self->pos];

  if (lexer_is_newline(c)) {
    lexer_advance_byte(self);
    lexer_emit(self, TOKEN_TYPE_NEWLINE, begin, line, col);
    return;
  }

  if (lexer_at(self, self->pos, "#;")) {
    lexer_advance_byte(self);
    lexer_advance_byte(self);
    lexer_emit(self, TOKEN_TYPE_STRUCTURED_COMMENT, begin, line, col);
    return;
  }

  if (c == '{') {
    const bool glued = lexer_glued(self, begin);
    size_t depth = 0;
    do {
      if (self->text.data[self->pos] == '{') { ++depth; }
      if (self->text.data[self->pos] == '}') {
        --depth;
        lexer_advance_byte(self);
        if (depth == 0) {
          lexer_emit(
              self, glued ? TOKEN_TYPE_G_STRING : TOKEN_TYPE_STRING_LITERAL, begin, line, col);
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
    while (self->pos < self->text.len && lexer_is_digit(self->text.data[self->pos])) {
      lexer_advance_byte(self);
    }
    // integer_literal ::= [1-9][0-9]* | "0"
    if (c == '0' && self->pos != begin + 1) {
      lexer_set_error(self, begin, line, col);
      return;
    }
    lexer_emit(self, TOKEN_TYPE_INTEGER_LITERAL, begin, line, col);
    return;
  }

  if (lexer_is_ident_start(c)) {
    lexer_advance_byte(self);
    while (self->pos < self->text.len && lexer_is_ident_continue(self->text.data[self->pos])) {
      lexer_advance_byte(self);
    }
    if (lexer_text_eq(self->text, begin, self->pos, "do", 2)) {
      lexer_emit(self, TOKEN_TYPE_KW_DO, begin, line, col);
    } else if (lexer_text_eq(self->text, begin, self->pos, "end", 3)) {
      lexer_emit(self, TOKEN_TYPE_KW_END, begin, line, col);
    } else {
      lexer_emit(self, TOKEN_TYPE_IDENTIFIER, begin, line, col);
    }
    return;
  }

  switch (c) {
    case '@':
      lexer_advance_byte(self);
      if (self->pos < self->text.len && self->text.data[self->pos] == '[') {
        lexer_advance_byte(self);
        lexer_emit(self, TOKEN_TYPE_ANNOT_OPEN, begin, line, col);
      } else {
        lexer_set_error(self, begin, line, col);
      }
      return;
    case '(':
    case '[': {
      const bool glued = lexer_glued(self, begin);
      lexer_advance_byte(self);
      u8 type = c == '(' ? (glued ? TOKEN_TYPE_G_LPAREN : TOKEN_TYPE_LPAREN)
                         : (glued ? TOKEN_TYPE_G_LBRACKET : TOKEN_TYPE_LBRACKET);
      lexer_emit(self, type, begin, line, col);
      return;
    }
    case ')': lexer_advance_byte(self); lexer_emit(self, TOKEN_TYPE_RPAREN, begin, line, col); return;
    case ']':
      lexer_advance_byte(self);
      lexer_emit(self, TOKEN_TYPE_RBRACKET, begin, line, col);
      return;
    case ',': lexer_advance_byte(self); lexer_emit(self, TOKEN_TYPE_COMMA, begin, line, col); return;
    case ';':
      lexer_advance_byte(self);
      lexer_emit(self, TOKEN_TYPE_SEMICOLON, begin, line, col);
      return;
    case '.':
      lexer_advance_byte(self);
      if (lexer_glued(self, begin)) {
        lexer_emit(self, TOKEN_TYPE_G_DOT, begin, line, col);
      } else {
        lexer_set_error(self, begin, line, col);
      }
      return;
    case '~': lexer_advance_byte(self); lexer_emit(self, TOKEN_TYPE_TILDE, begin, line, col); return;
    case '$': lexer_advance_byte(self); lexer_emit(self, TOKEN_TYPE_DOLLAR, begin, line, col); return;
    default: break;
  }

  if (lexer_is_operator_char(c)) {
    lexer_lex_operator_run(self, begin, line, col);
    return;
  }

  lexer_advance_byte(self);
  lexer_set_error(self, begin, line, col);
}

// Expression-ending tokens: literal, identifier, `)`, `]`, `end`.
static bool lexer_token_ends_expression(token_t token) {
  switch (token.type.value) {
    case TOKEN_TYPE_STRING_LITERAL:
    case TOKEN_TYPE_G_STRING:
    case TOKEN_TYPE_INTEGER_LITERAL:
    case TOKEN_TYPE_IDENTIFIER:
    case TOKEN_TYPE_RPAREN:
    case TOKEN_TYPE_RBRACKET:
    case TOKEN_TYPE_KW_END: return true;
    default: return false;
  }
}

static bool lexer_layout_active(struct lexer_t const* self) {
  return self->layout_depth == 0 || self->layout[self->layout_depth - 1] == LAYOUT_BLOCK;
}

static void lexer_layout_push(struct lexer_t* self, u8 kind) {
  if (self->layout_depth >= LEXER_LAYOUT_MAX) {
    self->error = ERROR_OVERFLOW;
    self->cur.type = CTOR(token_type_t, TOKEN_TYPE_ERROR);
    return;
  }
  self->layout[self->layout_depth++] = kind;
}

static void lexer_layout_pop(struct lexer_t* self) {
  if (self->layout_depth != 0) { --self->layout_depth; }
}

// Track layout contexts and the facts gluing and ASI need about the previous token.
static void lexer_note_token(struct lexer_t* self) {
  // `#;` is trivia: it leaves the previous significant token in place.
  if (self->cur.type.value == TOKEN_TYPE_STRUCTURED_COMMENT) { return; }
  bool label_word = false;
  switch (self->cur.type.value) {
    case TOKEN_TYPE_LPAREN:
    case TOKEN_TYPE_G_LPAREN: lexer_layout_push(self, LAYOUT_PAREN); break;
    case TOKEN_TYPE_LBRACKET:
    case TOKEN_TYPE_G_LBRACKET: lexer_layout_push(self, LAYOUT_BRACKET); break;
    case TOKEN_TYPE_ANNOT_OPEN: lexer_layout_push(self, LAYOUT_ANNOT); break;
    case TOKEN_TYPE_RPAREN:
    case TOKEN_TYPE_RBRACKET: lexer_layout_pop(self); break;
    case TOKEN_TYPE_KW_DO:
      label_word = lexer_label_word_follows(self);
      if (!label_word) { lexer_layout_push(self, LAYOUT_BLOCK); }
      break;
    case TOKEN_TYPE_KW_END:
      label_word = lexer_label_word_follows(self);
      if (!label_word) { lexer_layout_pop(self); }
      break;
    case TOKEN_TYPE_IDENTIFIER: label_word = true; break;
    default: break;
  }
  if (self->error != ERROR_SUCCESS) { return; }
  self->prev = self->cur;
  self->has_prev = true;
  self->last_end = self->cur.end;
  // `end:` is a label word, not the end of an expression.
  self->last_ends_expr = lexer_token_ends_expression(self->cur)
                         && !(self->cur.type.value == TOKEN_TYPE_KW_END && label_word);
  self->last_is_label_word = label_word;
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
      lexer_note_token(self);
      return;
    }

    // A physical newline becomes a virtual `;` iff the context is layout-active and the
    // previous significant token can end an expression. `...` was consumed as trivia.
    if (lexer_layout_active(self) && self->has_prev && lexer_token_ends_expression(self->prev)) {
      self->cur.type = CTOR(token_type_t, TOKEN_TYPE_SEMICOLON);
      self->prev = self->cur;
      self->last_end = self->cur.end;
      self->last_ends_expr = false;
      self->last_is_label_word = false;
      return;
    }
  }
}
