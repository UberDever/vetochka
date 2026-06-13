#ifndef REDUCER_SOURCE_API_H
#define REDUCER_SOURCE_API_H

#include "allocator_api.h"
#include "domain_api.h"

#define SOURCE_NODE_TYPE_ITEMS(X, P)                                                               \
  X(P, EOF, 0, "eof")                                                                              \
  X(P, ERROR, 1, "error")                                                                          \
  X(P, SOURCE, 2, "source")                                                                        \
  X(P, BLOCK_LIST, 3, "block-list")                                                                \
  X(P, EXPRESSION, 4, "expression")                                                                \
  X(P, ANNOTATION, 5, "annotation")                                                                \
  X(P, INFIX_EXPRESSION, 6, "infix-expression")                                                    \
  X(P, PREFIX_EXPRESSION, 7, "prefix-expression")                                                  \
  X(P, POSTFIX_EXPRESSION, 8, "postfix-expression")                                                \
  X(P, TIGHT_POSTFIX, 9, "tight-postfix")                                                          \
  X(P, LOOSE_POSTFIX, 10, "loose-postfix")                                                         \
  X(P, BLOCK_ARGUMENT, 11, "block-argument")                                                       \
  X(P, LABELED_ARGUMENT, 12, "labeled-argument")                                                   \
  X(P, ARGUMENT_EXPRESSION, 13, "argument-expression")                                             \
  X(P, INFIX_EXPRESSION_TIGHT, 14, "infix-expression-tight")                                       \
  X(P, PREFIX_EXPRESSION_TIGHT, 15, "prefix-expression-tight")                                     \
  X(P, POSTFIX_EXPRESSION_TIGHT, 16, "postfix-expression-tight")                                   \
  X(P, PRIMARY, 17, "primary")                                                                     \
  X(P, COMMA_LIST, 18, "comma-list")                                                               \
  X(P, TOKEN, 19, "token")                                                                         \
  X(P, IMPLICIT_DELTA, 20, "implicit-delta")

DECL_TYPED_ENUM(source_node_type_t, u8, SOURCE_NODE_TYPE, SOURCE_NODE_TYPE_ITEMS)

struct source_token_t {
  size_t begin;
  size_t end;
  size_t line;
  size_t col;
};

struct source_node_t {
  struct source_token_t token;
  source_node_type_t type;
  size_t parent_index;
  size_t child_index;
  size_t next_index;
};

struct source_tree_t;

MUH_PUBLIC error_t
source_ast_create(struct source_tree_t** out_tree, span_cbyte_t text, struct allocator_t allocator);
MUH_PUBLIC void source_tree_free(struct source_tree_t** tree);

MUH_PUBLIC size_t source_tree_get_count(const struct source_tree_t* tree);
MUH_PUBLIC struct source_node_t source_tree_get_node(
    const struct source_tree_t* tree, size_t index);

MUH_PUBLIC error_t source_tree_format_sexpr(
    span_cbyte_t text, const struct source_tree_t* tree, struct da_byte_t* out);
MUH_PUBLIC error_t source_tree_format_canonical(
    span_cbyte_t text, const struct source_tree_t* tree, struct da_byte_t* out);

#endif // REDUCER_SOURCE_API_H
