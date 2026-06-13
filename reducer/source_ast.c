#include "source_api.h"
#include "vendor/da.h"

#include <stddef.h>

#define AST_NO_NODE SIZE_MAX

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

MUH_PRIVATE void lexer_next(struct lexer_t* self);

struct source_tree_t {
  struct allocator_t allocator;
  struct source_node_t* items;
  size_t count;
  size_t capacity;
};

struct parser_t {
  struct lexer_t lexer;
};

static struct source_token_t ast_public_token(token_t token) {
  return CTOR(
      struct source_token_t,
      .begin = token.begin,
      .end = token.end,
      .line = token.line,
      .col = token.col);
}

static bool ast_token_text_eq(
    const struct lexer_t* lexer, token_t token, const char* text, size_t len) {
  if (token.end - token.begin != len) { return false; }
  for (size_t i = 0; i < len; ++i) {
    if (lexer->text.data[token.begin + i] != (byte)text[i]) { return false; }
  }
  return true;
}

static bool ast_cur_is(struct parser_t* parser, token_type_t type) {
  return parser->lexer.cur.type.value == type.value;
}

static bool ast_cur_is_text(
    struct parser_t* parser, token_type_t type, const char* text, size_t len) {
  return ast_cur_is(parser, type)
         && ast_token_text_eq(&parser->lexer, parser->lexer.cur, text, len);
}

static bool ast_is_delim(struct parser_t* parser, const char* text, size_t len) {
  return ast_cur_is_text(parser, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), text, len);
}

static void ast_next(struct parser_t* parser) {
  lexer_next(&parser->lexer);
}

static size_t ast_add_node(struct source_tree_t* tree, source_node_type_t type, token_t token) {
  const size_t index = tree->count;
  da_append(
      tree,
      CTOR(
          struct source_node_t,
          .token = ast_public_token(token),
          .type = type,
          .parent_index = AST_NO_NODE,
          .child_index = AST_NO_NODE,
          .next_index = AST_NO_NODE));
  return index;
}

static size_t ast_add_virtual_node(
    struct source_tree_t* tree, source_node_type_t type, token_t token) {
  return ast_add_node(tree, type, token);
}

static void ast_add_child(struct source_tree_t* tree, size_t parent, size_t child) {
  if (parent == AST_NO_NODE || child == AST_NO_NODE) { return; }
  tree->items[child].parent_index = parent;
  if (tree->items[parent].child_index == AST_NO_NODE) {
    tree->items[parent].child_index = child;
    return;
  }
  size_t cursor = tree->items[parent].child_index;
  while (tree->items[cursor].next_index != AST_NO_NODE) {
    cursor = tree->items[cursor].next_index;
  }
  tree->items[cursor].next_index = child;
}

static size_t ast_add_token_node(struct source_tree_t* tree, token_t token) {
  return ast_add_node(tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_TOKEN), token);
}

static error_t ast_expect(
    struct parser_t* parser, struct source_tree_t* tree, token_type_t type, size_t* out) {
  if (!ast_cur_is(parser, type)) { return ERROR_GENERIC; }
  *out = ast_add_token_node(tree, parser->lexer.cur);
  ast_next(parser);
  return parser->lexer.error;
}

static error_t ast_expect_text(
    struct parser_t* parser,
    struct source_tree_t* tree,
    token_type_t type,
    const char* text,
    size_t len,
    size_t* out) {
  if (!ast_cur_is_text(parser, type, text, len)) { return ERROR_GENERIC; }
  *out = ast_add_token_node(tree, parser->lexer.cur);
  ast_next(parser);
  return parser->lexer.error;
}

static bool ast_is_expression_start(struct parser_t* parser) {
  if (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_STRUCTURED_COMMENT))) { return true; }
  if (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_STRING_LITERAL))) { return true; }
  if (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_INTEGER_LITERAL))) { return true; }
  if (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_DELTA_NODE))) { return true; }
  if (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_IDENTIFIER))) { return true; }
  if (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_UNARY))) { return true; }
  return ast_is_delim(parser, "@[", 2) || ast_is_delim(parser, "[", 1)
         || ast_is_delim(parser, "(", 1);
}

static bool ast_is_block_end(struct parser_t* parser) {
  return ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_EOF))
         || ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_KW_END))
         || ast_is_delim(parser, ")", 1) || ast_is_delim(parser, "]", 1);
}

static error_t ast_parse_expression(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out);
static error_t ast_parse_infix_expression_tight(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out);

static error_t ast_skip_structured_comments(struct parser_t* parser, struct source_tree_t* tree) {
  while (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_STRUCTURED_COMMENT))) {
    ast_next(parser);
    size_t discarded = AST_NO_NODE;
    error_t err = ast_parse_expression(parser, tree, &discarded);
    if (err != ERROR_SUCCESS) { return err; }
    (void)discarded;
  }
  return ERROR_SUCCESS;
}

static error_t ast_parse_comma_list(
    struct parser_t* parser,
    struct source_tree_t* tree,
    const char* close,
    size_t close_len,
    size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_COMMA_LIST), parser->lexer.cur);
  if (ast_is_delim(parser, close, close_len)) {
    *out = node;
    return ERROR_SUCCESS;
  }

  for (;;) {
    size_t expr = AST_NO_NODE;
    error_t err = ast_parse_expression(parser, tree, &expr);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, expr);

    if (!ast_is_delim(parser, ",", 1)) { break; }
    size_t comma = AST_NO_NODE;
    err = ast_expect_text(parser, tree, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), ",", 1, &comma);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, comma);
    if (ast_is_delim(parser, close, close_len)) { break; }
  }

  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_block_list(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_BLOCK_LIST), parser->lexer.cur);
  if (ast_is_block_end(parser)) {
    *out = node;
    return ERROR_SUCCESS;
  }

  for (;;) {
    if (!ast_is_expression_start(parser)) { return ERROR_GENERIC; }
    size_t expr = AST_NO_NODE;
    error_t err = ast_parse_expression(parser, tree, &expr);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, expr);

    if (!ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_SEMICOLON))) { break; }
    size_t semi = AST_NO_NODE;
    err = ast_expect(parser, tree, CTOR(token_type_t, TOKEN_TYPE_SEMICOLON), &semi);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, semi);
    if (ast_is_block_end(parser)) { break; }
  }

  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_annotation(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_ANNOTATION), parser->lexer.cur);
  size_t open = AST_NO_NODE;
  error_t err =
      ast_expect_text(parser, tree, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), "@[", 2, &open);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, open);

  size_t expr = AST_NO_NODE;
  err = ast_parse_expression(parser, tree, &expr);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, expr);

  size_t close = AST_NO_NODE;
  err = ast_expect_text(parser, tree, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), "]", 1, &close);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, close);
  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_primary(struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  error_t err = ast_skip_structured_comments(parser, tree);
  if (err != ERROR_SUCCESS) { return err; }

  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_PRIMARY), parser->lexer.cur);

  if (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_STRING_LITERAL))
      || ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_INTEGER_LITERAL))
      || ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_DELTA_NODE))
      || ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_IDENTIFIER))) {
    size_t token = ast_add_token_node(tree, parser->lexer.cur);
    ast_add_child(tree, node, token);
    ast_next(parser);
    *out = node;
    return parser->lexer.error;
  }

  if (ast_is_delim(parser, "[", 1)) {
    size_t open = AST_NO_NODE;
    err = ast_expect_text(parser, tree, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), "[", 1, &open);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, open);

    size_t list = AST_NO_NODE;
    err = ast_parse_comma_list(parser, tree, "]", 1, &list);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, list);

    size_t close = AST_NO_NODE;
    err = ast_expect_text(parser, tree, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), "]", 1, &close);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, close);
    *out = node;
    return ERROR_SUCCESS;
  }

  if (ast_is_delim(parser, "(", 1)) {
    size_t open = AST_NO_NODE;
    err = ast_expect_text(parser, tree, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), "(", 1, &open);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, open);

    size_t expr = AST_NO_NODE;
    err = ast_parse_expression(parser, tree, &expr);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, expr);

    size_t close = AST_NO_NODE;
    err = ast_expect_text(parser, tree, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), ")", 1, &close);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, close);
    *out = node;
    return ERROR_SUCCESS;
  }

  return ERROR_GENERIC;
}

static error_t ast_parse_tight_postfix(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_TIGHT_POSTFIX), parser->lexer.cur);
  error_t err = ERROR_SUCCESS;

  if (ast_is_delim(parser, ".", 1)) {
    size_t dot = AST_NO_NODE;
    err = ast_expect_text(parser, tree, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), ".", 1, &dot);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, dot);
    size_t ident = AST_NO_NODE;
    err = ast_expect(parser, tree, CTOR(token_type_t, TOKEN_TYPE_IDENTIFIER), &ident);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, ident);
    *out = node;
    return ERROR_SUCCESS;
  }

  if (ast_is_delim(parser, "(", 1) || ast_is_delim(parser, "[", 1)) {
    const char* open_text = ast_is_delim(parser, "(", 1) ? "(" : "[";
    const char* close_text = open_text[0] == '(' ? ")" : "]";
    size_t open = AST_NO_NODE;
    err = ast_expect_text(
        parser, tree, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), open_text, 1, &open);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, open);

    if (ast_is_delim(parser, close_text, 1) && open_text[0] == '(') {
      size_t implicit = ast_add_node(
          tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_IMPLICIT_DELTA), parser->lexer.cur);
      ast_add_child(tree, node, implicit);
    } else {
      size_t list = AST_NO_NODE;
      err = ast_parse_comma_list(parser, tree, close_text, 1, &list);
      if (err != ERROR_SUCCESS) { return err; }
      ast_add_child(tree, node, list);
    }

    size_t close = AST_NO_NODE;
    err = ast_expect_text(
        parser, tree, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), close_text, 1, &close);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, close);
    *out = node;
    return ERROR_SUCCESS;
  }

  if (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_STRING_LITERAL))) {
    size_t string = ast_add_token_node(tree, parser->lexer.cur);
    ast_add_child(tree, node, string);
    ast_next(parser);
    *out = node;
    return parser->lexer.error;
  }

  return ERROR_GENERIC;
}

static error_t ast_parse_block_argument(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_BLOCK_ARGUMENT), parser->lexer.cur);
  size_t do_node = AST_NO_NODE;
  error_t err = ast_expect(parser, tree, CTOR(token_type_t, TOKEN_TYPE_KW_DO), &do_node);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, do_node);

  if (!ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_KW_END))) {
    size_t block = AST_NO_NODE;
    err = ast_parse_block_list(parser, tree, &block);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, block);
  }

  size_t end_node = AST_NO_NODE;
  err = ast_expect(parser, tree, CTOR(token_type_t, TOKEN_TYPE_KW_END), &end_node);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, end_node);
  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_argument_expression(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_ARGUMENT_EXPRESSION), parser->lexer.cur);
  if (ast_is_delim(parser, "@[", 2)) {
    size_t ann = AST_NO_NODE;
    error_t err = ast_parse_annotation(parser, tree, &ann);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, ann);
  }
  size_t expr = AST_NO_NODE;
  error_t err = ast_parse_infix_expression_tight(parser, tree, &expr);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, expr);
  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_labeled_argument(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_LABELED_ARGUMENT), parser->lexer.cur);
  size_t ident = AST_NO_NODE;
  error_t err = ast_expect(parser, tree, CTOR(token_type_t, TOKEN_TYPE_IDENTIFIER), &ident);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, ident);
  size_t colon = AST_NO_NODE;
  err = ast_expect_text(parser, tree, CTOR(token_type_t, TOKEN_TYPE_DELIMETER), ":", 1, &colon);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, colon);
  size_t arg = AST_NO_NODE;
  err = ast_parse_argument_expression(parser, tree, &arg);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, arg);
  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_postfix_expression_tight(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_POSTFIX_EXPRESSION_TIGHT), parser->lexer.cur);
  size_t primary = AST_NO_NODE;
  error_t err = ast_parse_primary(parser, tree, &primary);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, primary);

  while (ast_is_delim(parser, ".", 1) || ast_is_delim(parser, "(", 1)
         || ast_is_delim(parser, "[", 1)
         || ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_STRING_LITERAL))) {
    size_t postfix = AST_NO_NODE;
    err = ast_parse_tight_postfix(parser, tree, &postfix);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, postfix);
  }

  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_prefix_expression_tight(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_PREFIX_EXPRESSION_TIGHT), parser->lexer.cur);
  while (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_UNARY))) {
    size_t unary = ast_add_token_node(tree, parser->lexer.cur);
    ast_add_child(tree, node, unary);
    ast_next(parser);
    if (parser->lexer.error != ERROR_SUCCESS) { return parser->lexer.error; }
  }
  size_t postfix = AST_NO_NODE;
  error_t err = ast_parse_postfix_expression_tight(parser, tree, &postfix);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, postfix);
  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_infix_expression_tight(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_INFIX_EXPRESSION_TIGHT), parser->lexer.cur);
  size_t lhs = AST_NO_NODE;
  error_t err = ast_parse_prefix_expression_tight(parser, tree, &lhs);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, lhs);

  while (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_OPERATOR))
         || ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_UNARY))) {
    size_t op = ast_add_token_node(tree, parser->lexer.cur);
    ast_add_child(tree, node, op);
    ast_next(parser);
    if (parser->lexer.error != ERROR_SUCCESS) { return parser->lexer.error; }
    size_t rhs = AST_NO_NODE;
    err = ast_parse_prefix_expression_tight(parser, tree, &rhs);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, rhs);
  }

  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_postfix_expression(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_POSTFIX_EXPRESSION), parser->lexer.cur);
  size_t primary = AST_NO_NODE;
  error_t err = ast_parse_primary(parser, tree, &primary);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, primary);

  while (ast_is_delim(parser, ".", 1) || ast_is_delim(parser, "(", 1)
         || ast_is_delim(parser, "[", 1)
         || ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_STRING_LITERAL))) {
    size_t postfix = AST_NO_NODE;
    err = ast_parse_tight_postfix(parser, tree, &postfix);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, postfix);
  }

  while (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_KW_DO))
         || ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_IDENTIFIER))) {
    size_t loose = ast_add_virtual_node(
        tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_LOOSE_POSTFIX), parser->lexer.cur);
    size_t arg = AST_NO_NODE;
    if (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_KW_DO))) {
      err = ast_parse_block_argument(parser, tree, &arg);
    } else {
      err = ast_parse_labeled_argument(parser, tree, &arg);
    }
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, loose, arg);
    ast_add_child(tree, node, loose);
  }

  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_prefix_expression(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_PREFIX_EXPRESSION), parser->lexer.cur);
  while (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_UNARY))) {
    size_t unary = ast_add_token_node(tree, parser->lexer.cur);
    ast_add_child(tree, node, unary);
    ast_next(parser);
    if (parser->lexer.error != ERROR_SUCCESS) { return parser->lexer.error; }
  }
  size_t postfix = AST_NO_NODE;
  error_t err = ast_parse_postfix_expression(parser, tree, &postfix);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, postfix);
  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_infix_expression(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_INFIX_EXPRESSION), parser->lexer.cur);
  size_t lhs = AST_NO_NODE;
  error_t err = ast_parse_prefix_expression(parser, tree, &lhs);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, lhs);

  while (ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_OPERATOR))
         || ast_cur_is(parser, CTOR(token_type_t, TOKEN_TYPE_UNARY))) {
    size_t op = ast_add_token_node(tree, parser->lexer.cur);
    ast_add_child(tree, node, op);
    ast_next(parser);
    if (parser->lexer.error != ERROR_SUCCESS) { return parser->lexer.error; }
    size_t rhs = AST_NO_NODE;
    err = ast_parse_prefix_expression(parser, tree, &rhs);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, rhs);
  }

  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_expression(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  error_t err = ast_skip_structured_comments(parser, tree);
  if (err != ERROR_SUCCESS) { return err; }

  const size_t node = ast_add_virtual_node(
      tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_EXPRESSION), parser->lexer.cur);
  if (ast_is_delim(parser, "@[", 2)) {
    size_t ann = AST_NO_NODE;
    err = ast_parse_annotation(parser, tree, &ann);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, ann);
  }

  size_t infix = AST_NO_NODE;
  err = ast_parse_infix_expression(parser, tree, &infix);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, infix);
  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_with_parser(const struct parser_t* self, struct source_tree_t* out_tree) {
  if (self == NULL || out_tree == NULL) { return ERROR_INVALID_PARAM; }

  struct parser_t parser = *self;
  ast_next(&parser);
  if (parser.lexer.error != ERROR_SUCCESS) { return parser.lexer.error; }

  const size_t source = ast_add_virtual_node(
      out_tree, CTOR(source_node_type_t, SOURCE_NODE_TYPE_SOURCE), parser.lexer.cur);
  if (!ast_cur_is(&parser, CTOR(token_type_t, TOKEN_TYPE_EOF))) {
    size_t block = AST_NO_NODE;
    error_t err = ast_parse_block_list(&parser, out_tree, &block);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(out_tree, source, block);
  }

  size_t eof = AST_NO_NODE;
  error_t err = ast_expect(&parser, out_tree, CTOR(token_type_t, TOKEN_TYPE_EOF), &eof);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(out_tree, source, eof);
  return ERROR_SUCCESS;
}

error_t source_ast_create(
    struct source_tree_t** out_tree, span_cbyte_t text, struct allocator_t allocator) {
  if (out_tree == NULL) { return ERROR_INVALID_PARAM; }
  *out_tree = NULL;
  if ((text.len != 0 && text.data == NULL) || allocator.vtable == NULL
      || allocator.vtable->alloc == NULL || allocator.vtable->remap == NULL
      || allocator.vtable->free == NULL) {
    return ERROR_INVALID_PARAM;
  }

  struct source_tree_t* tree = allocator.vtable->alloc(allocator.ctx, sizeof(*tree), ALIGNMENT_MAX);
  if (tree == NULL) { return ERROR_NOMEM; }
  *tree = CTOR(struct source_tree_t, .allocator = allocator);

  struct parser_t parser = {0};
  parser.lexer.text = text;
  parser.lexer.error = ERROR_SUCCESS;
  error_t err = ast_parse_with_parser(&parser, tree);
  if (err != ERROR_SUCCESS) {
    da_free(tree);
    allocator.vtable->free(allocator.ctx, tree, sizeof(*tree), ALIGNMENT_MAX);
    return err;
  }

  *out_tree = tree;
  return ERROR_SUCCESS;
}

void source_tree_free(struct source_tree_t** tree) {
  if (tree == NULL || *tree == NULL) { return; }
  struct source_tree_t* self = *tree;
  struct allocator_t allocator = self->allocator;
  da_free(self);
  allocator.vtable->free(allocator.ctx, self, sizeof(*self), ALIGNMENT_MAX);
  *tree = NULL;
}

size_t source_tree_get_count(const struct source_tree_t* tree) {
  return tree == NULL ? 0 : tree->count;
}

struct source_node_t source_tree_get_node(const struct source_tree_t* tree, size_t index) {
  if (tree == NULL || index >= tree->count) {
    return CTOR(
        struct source_node_t,
        .type = CTOR(source_node_type_t, SOURCE_NODE_TYPE_ERROR),
        .parent_index = AST_NO_NODE,
        .child_index = AST_NO_NODE,
        .next_index = AST_NO_NODE);
  }
  return tree->items[index];
}
