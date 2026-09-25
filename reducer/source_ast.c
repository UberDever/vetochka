#include "source_api.h"
#include "source_impl.h"
#include "vendor/da.h"

#include <stddef.h>

// Parser for the grammar in docs/new_spec/02_syntax.md, "Grammar".

#define AST_NO_NODE SIZE_MAX

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

static bool ast_cur_is(struct parser_t* parser, u8 type) {
  return parser->lexer.cur.type.value == type;
}

static token_t ast_peek(struct parser_t* parser) {
  struct lexer_t copy = parser->lexer;
  lexer_next(&copy);
  return copy.cur;
}

static void ast_next(struct parser_t* parser) {
  lexer_next(&parser->lexer);
}

static size_t ast_add_node(struct source_tree_t* tree, u8 type, token_t token) {
  const size_t index = tree->count;
  da_append(
      tree,
      CTOR(
          struct source_node_t,
          .token = ast_public_token(token),
          .type = CTOR(source_node_type_t, type),
          .parent_index = AST_NO_NODE,
          .child_index = AST_NO_NODE,
          .next_index = AST_NO_NODE));
  return index;
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

// Consume the current token as a leaf of `parent`, with the given node type.
static error_t ast_take_as(
    struct parser_t* parser, struct source_tree_t* tree, u8 token_type, u8 node_type, size_t parent) {
  if (!ast_cur_is(parser, token_type)) { return ERROR_GENERIC; }
  ast_add_child(tree, parent, ast_add_node(tree, node_type, parser->lexer.cur));
  ast_next(parser);
  return parser->lexer.error;
}

static error_t ast_take(
    struct parser_t* parser, struct source_tree_t* tree, u8 token_type, size_t parent) {
  return ast_take_as(parser, tree, token_type, SOURCE_NODE_TYPE_TOKEN, parent);
}

// label ::= identifier | "do" | "end", followed by a glued `:`.
static bool ast_at_label(struct parser_t* parser) {
  return (ast_cur_is(parser, TOKEN_TYPE_IDENTIFIER) || ast_cur_is(parser, TOKEN_TYPE_KW_DO)
          || ast_cur_is(parser, TOKEN_TYPE_KW_END))
         && ast_peek(parser).type.value == TOKEN_TYPE_G_COLON;
}

static bool ast_at_block_argument(struct parser_t* parser) {
  return ast_cur_is(parser, TOKEN_TYPE_KW_DO) && !ast_at_label(parser);
}

static bool ast_at_block_end(struct parser_t* parser) {
  return ast_cur_is(parser, TOKEN_TYPE_EOF) || ast_cur_is(parser, TOKEN_TYPE_RPAREN)
         || ast_cur_is(parser, TOKEN_TYPE_RBRACKET)
         || (ast_cur_is(parser, TOKEN_TYPE_KW_END) && !ast_at_label(parser));
}

static error_t ast_parse_entry(struct parser_t* parser, struct source_tree_t* tree, size_t* out);
static error_t ast_parse_expression(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out);
static error_t ast_parse_labeled_expression(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out);
static error_t ast_parse_block_argument(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out);

// structured_comment ::= "#;" trivia* expression — the expression is discarded.
static error_t ast_skip_structured_comments(struct parser_t* parser, struct source_tree_t* tree) {
  while (ast_cur_is(parser, TOKEN_TYPE_STRUCTURED_COMMENT)) {
    // Whether a newline here would have become `;` had the comment not been written.
    const bool before_ends = parser->lexer.has_prev && parser->lexer.last_ends_expr;
    ast_next(parser);
    if (parser->lexer.error != ERROR_SUCCESS) { return parser->lexer.error; }
    const size_t mark = tree->count;
    size_t discarded = AST_NO_NODE;
    error_t err = ast_parse_expression(parser, tree, &discarded);
    if (err != ERROR_SUCCESS) { return err; }
    tree->count = mark;
    // Drop a virtual `;` the discarded expression caused on its own.
    const token_t cur = parser->lexer.cur;
    const bool virtual_semicolon = cur.type.value == TOKEN_TYPE_SEMICOLON && cur.begin < cur.end
                                   && (parser->lexer.text.data[cur.begin] == '\n'
                                       || parser->lexer.text.data[cur.begin] == '\r');
    if (virtual_semicolon && !before_ends) {
      ast_next(parser);
      if (parser->lexer.error != ERROR_SUCCESS) { return parser->lexer.error; }
    }
  }
  return ERROR_SUCCESS;
}

// comma_list ::= entry ("," entry)* ","?
static error_t ast_parse_comma_list(
    struct parser_t* parser, struct source_tree_t* tree, u8 close, size_t* out) {
  const size_t node = ast_add_node(tree, SOURCE_NODE_TYPE_COMMA_LIST, parser->lexer.cur);
  for (;;) {
    size_t entry = AST_NO_NODE;
    error_t err = ast_parse_entry(parser, tree, &entry);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, entry);
    if (!ast_cur_is(parser, TOKEN_TYPE_COMMA)) { break; }
    err = ast_take(parser, tree, TOKEN_TYPE_COMMA, node);
    if (err != ERROR_SUCCESS) { return err; }
    if (ast_cur_is(parser, close)) { break; }
  }
  *out = node;
  return ERROR_SUCCESS;
}

// block_list ::= entry (";" entry)* ";"?
static error_t ast_parse_block_list(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_node(tree, SOURCE_NODE_TYPE_BLOCK_LIST, parser->lexer.cur);
  while (!ast_at_block_end(parser)) {
    size_t entry = AST_NO_NODE;
    error_t err = ast_parse_entry(parser, tree, &entry);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, entry);
    if (!ast_cur_is(parser, TOKEN_TYPE_SEMICOLON)) { break; }
    err = ast_take(parser, tree, TOKEN_TYPE_SEMICOLON, node);
    if (err != ERROR_SUCCESS) { return err; }
  }
  *out = node;
  return ERROR_SUCCESS;
}

// annotation ::= "@[" comma_list "]"
static error_t ast_parse_annotation(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_node(tree, SOURCE_NODE_TYPE_ANNOTATION, parser->lexer.cur);
  error_t err = ast_take(parser, tree, TOKEN_TYPE_ANNOT_OPEN, node);
  if (err != ERROR_SUCCESS) { return err; }
  size_t list = AST_NO_NODE;
  err = ast_parse_comma_list(parser, tree, TOKEN_TYPE_RBRACKET, &list);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, list);
  err = ast_take(parser, tree, TOKEN_TYPE_RBRACKET, node);
  if (err != ERROR_SUCCESS) { return err; }
  *out = node;
  return ERROR_SUCCESS;
}

// Bracketed comma list: open comma_list? close. An empty `()` records an implicit `~[]`.
static error_t ast_parse_bracketed(
    struct parser_t* parser, struct source_tree_t* tree, u8 open, u8 close, size_t node) {
  error_t err = ast_take(parser, tree, open, node);
  if (err != ERROR_SUCCESS) { return err; }
  if (ast_cur_is(parser, close)) {
    if (open == TOKEN_TYPE_G_LPAREN) {
      ast_add_child(tree, node, ast_add_node(tree, SOURCE_NODE_TYPE_IMPLICIT_NYAD, parser->lexer.cur));
    }
  } else {
    size_t list = AST_NO_NODE;
    err = ast_parse_comma_list(parser, tree, close, &list);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, list);
  }
  return ast_take(parser, tree, close, node);
}

// nyad ::= "~" "[" ( expression ( "," expression )? )? "]"
static error_t ast_parse_nyad(struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_node(tree, SOURCE_NODE_TYPE_NYAD, parser->lexer.cur);
  error_t err = ast_take(parser, tree, TOKEN_TYPE_TILDE, node);
  if (err != ERROR_SUCCESS) { return err; }
  err = ast_take(parser, tree, TOKEN_TYPE_LBRACKET, node);
  if (err != ERROR_SUCCESS) { return err; }
  for (size_t count = 0; !ast_cur_is(parser, TOKEN_TYPE_RBRACKET); ++count) {
    if (count == 2) { return ERROR_GENERIC; }
    if (count == 1) {
      err = ast_take(parser, tree, TOKEN_TYPE_COMMA, node);
      if (err != ERROR_SUCCESS) { return err; }
    }
    size_t child = AST_NO_NODE;
    err = ast_parse_expression(parser, tree, &child);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, child);
  }
  err = ast_take(parser, tree, TOKEN_TYPE_RBRACKET, node);
  if (err != ERROR_SUCCESS) { return err; }
  *out = node;
  return ERROR_SUCCESS;
}

// primary ::= literal | identifier | nyad | opcode | "[" comma_list? "]" | "(" entry ")"
static error_t ast_parse_primary(struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  error_t err = ast_skip_structured_comments(parser, tree);
  if (err != ERROR_SUCCESS) { return err; }

  if (ast_cur_is(parser, TOKEN_TYPE_TILDE)) { return ast_parse_nyad(parser, tree, out); }

  const size_t node = ast_add_node(tree, SOURCE_NODE_TYPE_PRIMARY, parser->lexer.cur);
  *out = node;
  switch (parser->lexer.cur.type.value) {
    case TOKEN_TYPE_STRING_LITERAL:
    case TOKEN_TYPE_INTEGER_LITERAL:
    case TOKEN_TYPE_IDENTIFIER: return ast_take(parser, tree, parser->lexer.cur.type.value, node);
    case TOKEN_TYPE_DOLLAR: {
      // opcode ::= special_dollar labeled_expression
      const size_t opcode = ast_add_node(tree, SOURCE_NODE_TYPE_OPCODE, parser->lexer.cur);
      ast_add_child(tree, node, opcode);
      err = ast_take(parser, tree, TOKEN_TYPE_DOLLAR, opcode);
      if (err != ERROR_SUCCESS) { return err; }
      if (!ast_at_label(parser)) { return ERROR_GENERIC; }
      size_t labeled = AST_NO_NODE;
      err = ast_parse_labeled_expression(parser, tree, &labeled);
      ast_add_child(tree, opcode, labeled);
      return err;
    }
    case TOKEN_TYPE_LBRACKET:
      return ast_parse_bracketed(parser, tree, TOKEN_TYPE_LBRACKET, TOKEN_TYPE_RBRACKET, node);
    case TOKEN_TYPE_LPAREN: {
      err = ast_take(parser, tree, TOKEN_TYPE_LPAREN, node);
      if (err != ERROR_SUCCESS) { return err; }
      size_t entry = AST_NO_NODE;
      err = ast_parse_entry(parser, tree, &entry);
      if (err != ERROR_SUCCESS) { return err; }
      ast_add_child(tree, node, entry);
      return ast_take(parser, tree, TOKEN_TYPE_RPAREN, node);
    }
    default: return ERROR_GENERIC;
  }
}

static bool ast_at_tight_postfix(struct parser_t* parser) {
  return ast_cur_is(parser, TOKEN_TYPE_G_DOT) || ast_cur_is(parser, TOKEN_TYPE_G_LPAREN)
         || ast_cur_is(parser, TOKEN_TYPE_G_LBRACKET) || ast_cur_is(parser, TOKEN_TYPE_G_STRING);
}

// tight_postfix ::= g_dot identifier | g_lparen comma_list? ")"
//                 | g_lbracket comma_list? "]" | g_string
static error_t ast_parse_tight_postfix(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_node(tree, SOURCE_NODE_TYPE_TIGHT_POSTFIX, parser->lexer.cur);
  *out = node;
  switch (parser->lexer.cur.type.value) {
    case TOKEN_TYPE_G_DOT: {
      error_t err = ast_take(parser, tree, TOKEN_TYPE_G_DOT, node);
      if (err != ERROR_SUCCESS) { return err; }
      return ast_take(parser, tree, TOKEN_TYPE_IDENTIFIER, node);
    }
    case TOKEN_TYPE_G_LPAREN:
      return ast_parse_bracketed(parser, tree, TOKEN_TYPE_G_LPAREN, TOKEN_TYPE_RPAREN, node);
    case TOKEN_TYPE_G_LBRACKET:
      return ast_parse_bracketed(parser, tree, TOKEN_TYPE_G_LBRACKET, TOKEN_TYPE_RBRACKET, node);
    case TOKEN_TYPE_G_STRING: return ast_take(parser, tree, TOKEN_TYPE_G_STRING, node);
    default: return ERROR_GENERIC;
  }
}

// postfix ::= primary tight_postfix* [loose_postfix*]
static error_t ast_parse_postfix(
    struct parser_t* parser, struct source_tree_t* tree, u8 type, bool loose, size_t* out) {
  const size_t node = ast_add_node(tree, type, parser->lexer.cur);
  size_t child = AST_NO_NODE;
  error_t err = ast_parse_primary(parser, tree, &child);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, child);

  while (ast_at_tight_postfix(parser)) {
    err = ast_parse_tight_postfix(parser, tree, &child);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, child);
  }

  // loose_postfix ::= block_argument | labeled_expression
  while (loose && (ast_at_block_argument(parser) || ast_at_label(parser))) {
    const size_t postfix = ast_add_node(tree, SOURCE_NODE_TYPE_LOOSE_POSTFIX, parser->lexer.cur);
    err = ast_at_block_argument(parser) ? ast_parse_block_argument(parser, tree, &child)
                                        : ast_parse_labeled_expression(parser, tree, &child);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, postfix, child);
    ast_add_child(tree, node, postfix);
  }

  *out = node;
  return ERROR_SUCCESS;
}

// prefix ::= op_prefix* postfix
static error_t ast_parse_prefix(
    struct parser_t* parser, struct source_tree_t* tree, bool tight, size_t* out) {
  const size_t node = ast_add_node(
      tree,
      tight ? SOURCE_NODE_TYPE_PREFIX_EXPRESSION_TIGHT : SOURCE_NODE_TYPE_PREFIX_EXPRESSION,
      parser->lexer.cur);
  while (ast_cur_is(parser, TOKEN_TYPE_OP_PREFIX)) {
    error_t err = ast_take_as(parser, tree, TOKEN_TYPE_OP_PREFIX, SOURCE_NODE_TYPE_OP_PREFIX, node);
    if (err != ERROR_SUCCESS) { return err; }
  }
  size_t postfix = AST_NO_NODE;
  error_t err = ast_parse_postfix(
      parser,
      tree,
      tight ? SOURCE_NODE_TYPE_POSTFIX_EXPRESSION_TIGHT : SOURCE_NODE_TYPE_POSTFIX_EXPRESSION,
      !tight,
      &postfix);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, postfix);
  *out = node;
  return ERROR_SUCCESS;
}

// infix ::= prefix (op_infix prefix)*
static error_t ast_parse_infix(
    struct parser_t* parser, struct source_tree_t* tree, bool tight, size_t* out) {
  const size_t node = ast_add_node(
      tree,
      tight ? SOURCE_NODE_TYPE_INFIX_EXPRESSION_TIGHT : SOURCE_NODE_TYPE_INFIX_EXPRESSION,
      parser->lexer.cur);
  size_t operand = AST_NO_NODE;
  error_t err = ast_parse_prefix(parser, tree, tight, &operand);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, operand);

  while (ast_cur_is(parser, TOKEN_TYPE_OP_INFIX)) {
    err = ast_take_as(parser, tree, TOKEN_TYPE_OP_INFIX, SOURCE_NODE_TYPE_OP_INFIX, node);
    if (err != ERROR_SUCCESS) { return err; }
    err = ast_parse_prefix(parser, tree, tight, &operand);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, operand);
  }
  *out = node;
  return ERROR_SUCCESS;
}

// annotation? infix, as `expression` or, tight, as `argument_expression`.
static error_t ast_parse_annotated(
    struct parser_t* parser, struct source_tree_t* tree, bool tight, size_t* out) {
  error_t err = ast_skip_structured_comments(parser, tree);
  if (err != ERROR_SUCCESS) { return err; }
  const size_t node = ast_add_node(
      tree,
      tight ? SOURCE_NODE_TYPE_ARGUMENT_EXPRESSION : SOURCE_NODE_TYPE_EXPRESSION,
      parser->lexer.cur);
  size_t child = AST_NO_NODE;
  if (ast_cur_is(parser, TOKEN_TYPE_ANNOT_OPEN)) {
    err = ast_parse_annotation(parser, tree, &child);
    if (err != ERROR_SUCCESS) { return err; }
    ast_add_child(tree, node, child);
  }
  err = ast_parse_infix(parser, tree, tight, &child);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, child);
  *out = node;
  return ERROR_SUCCESS;
}

static error_t ast_parse_expression(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  return ast_parse_annotated(parser, tree, false, out);
}

// labeled_expression ::= label g_colon argument_expression
static error_t ast_parse_labeled_expression(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_node(tree, SOURCE_NODE_TYPE_LABELED_EXPRESSION, parser->lexer.cur);
  error_t err = ast_take(parser, tree, parser->lexer.cur.type.value, node);
  if (err != ERROR_SUCCESS) { return err; }
  err = ast_take(parser, tree, TOKEN_TYPE_G_COLON, node);
  if (err != ERROR_SUCCESS) { return err; }
  size_t argument = AST_NO_NODE;
  err = ast_parse_annotated(parser, tree, true, &argument);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, argument);
  *out = node;
  return ERROR_SUCCESS;
}

// block_argument ::= "do" block_list? "end"
static error_t ast_parse_block_argument(
    struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  const size_t node = ast_add_node(tree, SOURCE_NODE_TYPE_BLOCK_ARGUMENT, parser->lexer.cur);
  error_t err = ast_take(parser, tree, TOKEN_TYPE_KW_DO, node);
  if (err != ERROR_SUCCESS) { return err; }
  size_t block = AST_NO_NODE;
  err = ast_parse_block_list(parser, tree, &block);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, node, block);
  err = ast_take(parser, tree, TOKEN_TYPE_KW_END, node);
  if (err != ERROR_SUCCESS) { return err; }
  *out = node;
  return ERROR_SUCCESS;
}

// entry ::= labeled_expression | block_argument | expression
static error_t ast_parse_entry(struct parser_t* parser, struct source_tree_t* tree, size_t* out) {
  error_t err = ast_skip_structured_comments(parser, tree);
  if (err != ERROR_SUCCESS) { return err; }
  if (ast_at_label(parser)) { return ast_parse_labeled_expression(parser, tree, out); }
  if (ast_at_block_argument(parser)) { return ast_parse_block_argument(parser, tree, out); }
  return ast_parse_expression(parser, tree, out);
}

// source ::= block_list? eof
static error_t ast_parse_source(struct parser_t* parser, struct source_tree_t* tree) {
  ast_next(parser);
  if (parser->lexer.error != ERROR_SUCCESS) { return parser->lexer.error; }

  const size_t source = ast_add_node(tree, SOURCE_NODE_TYPE_SOURCE, parser->lexer.cur);
  size_t block = AST_NO_NODE;
  error_t err = ast_parse_block_list(parser, tree, &block);
  if (err != ERROR_SUCCESS) { return err; }
  ast_add_child(tree, source, block);
  if (!ast_cur_is(parser, TOKEN_TYPE_EOF)) { return ERROR_GENERIC; }
  ast_add_child(tree, source, ast_add_node(tree, SOURCE_NODE_TYPE_TOKEN, parser->lexer.cur));
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
  error_t err = ast_parse_source(&parser, tree);
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
