#ifndef REDUCER_SOURCE_IMPL_H
#define REDUCER_SOURCE_IMPL_H

#include "domain_api.h"

// Token classes follow docs/new_spec/02_syntax.md, "Tokens". A `g_` class is glued:
// it starts right where an expression-ending token ends, with no trivia between.
#define TOKEN_TYPE_ITEMS(X, P)                                                                     \
  X(P, EOF, 0, "eof")                                                                              \
  X(P, ERROR, 1, "error")                                                                          \
  X(P, STRUCTURED_COMMENT, 2, "structured comment")                                                \
  X(P, NEWLINE, 3, "newline")                                                                      \
  X(P, STRING_LITERAL, 4, "string literal")                                                        \
  X(P, G_STRING, 5, "glued string")                                                                \
  X(P, INTEGER_LITERAL, 6, "integer literal")                                                      \
  X(P, IDENTIFIER, 7, "identifier")                                                                \
  X(P, OP_INFIX, 8, "infix operator")                                                              \
  X(P, OP_PREFIX, 9, "prefix operator")                                                            \
  X(P, DOLLAR, 10, "$")                                                                            \
  X(P, TILDE, 11, "~")                                                                             \
  X(P, LPAREN, 12, "(")                                                                            \
  X(P, G_LPAREN, 13, "glued (")                                                                    \
  X(P, RPAREN, 14, ")")                                                                            \
  X(P, LBRACKET, 15, "[")                                                                          \
  X(P, G_LBRACKET, 16, "glued [")                                                                  \
  X(P, RBRACKET, 17, "]")                                                                          \
  X(P, ANNOT_OPEN, 18, "@[")                                                                       \
  X(P, COMMA, 19, ",")                                                                             \
  X(P, G_DOT, 20, "glued .")                                                                       \
  X(P, G_COLON, 21, "glued :")                                                                     \
  X(P, SEMICOLON, 22, ";")                                                                         \
  X(P, KW_DO, 23, "do")                                                                            \
  X(P, KW_END, 24, "end")

DECL_TYPED_ENUM(token_type_t, u8, TOKEN_TYPE, TOKEN_TYPE_ITEMS)

typedef struct token_t {
  token_type_t type;
  size_t begin, end, line, col;
} token_t;

#define LEXER_LAYOUT_MAX 64u

struct lexer_t {
  span_cbyte_t text;
  size_t pos;
  size_t line;
  size_t col;
  token_t cur;
  token_t prev;           // previous significant token, for ASI
  size_t last_end;        // end offset of the previous significant token
  bool last_ends_expr;    // previous significant token can end an expression
  bool last_is_label_word;
  size_t layout_depth;
  u8 layout[LEXER_LAYOUT_MAX];
  bool has_prev;
  bool at_eof;
  error_t error;
};

MUH_PRIVATE void lexer_next(struct lexer_t* self);

#endif // REDUCER_SOURCE_IMPL_H
