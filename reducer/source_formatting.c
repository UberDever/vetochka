#include "source_api.h"
#include "vendor/da.h"

#include <stdbool.h>
#include <stddef.h>

#define TREE_NO_NODE   SIZE_MAX
#define SEXPR_INDENT   2u
#define CANON_LINE_MAX 80u

typedef struct fmt_t {
  struct da_byte_t* out;
  size_t col;
} fmt_t;

static error_t fmt_byte(fmt_t* fmt, byte c) {
  da_append(fmt->out, c);
  fmt->col = c == '\n' ? 0 : fmt->col + 1;
  return ERROR_SUCCESS;
}

static error_t fmt_text(fmt_t* fmt, const char* text) {
  for (size_t i = 0; text[i] != '\0'; ++i) {
    error_t err = fmt_byte(fmt, (byte)text[i]);
    if (err != ERROR_SUCCESS) { return err; }
  }
  return ERROR_SUCCESS;
}

static error_t fmt_indent(fmt_t* fmt, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    error_t err = fmt_byte(fmt, ' ');
    if (err != ERROR_SUCCESS) { return err; }
  }
  return ERROR_SUCCESS;
}

static size_t cstr_len(const char* s) {
  size_t n = 0;
  while (s[n] != '\0') {
    ++n;
  }
  return n;
}

static bool token_empty(struct source_token_t token) {
  return token.begin == token.end;
}

static size_t token_len(span_cbyte_t text, struct source_token_t token) {
  return token.end <= text.len ? token.end - token.begin : text.len - token.begin;
}

static bool token_eq(span_cbyte_t text, struct source_token_t token, const char* s) {
  const size_t len = cstr_len(s);
  if (token_len(text, token) != len) { return false; }
  for (size_t i = 0; i < len; ++i) {
    if (token.begin + i >= text.len || text.data[token.begin + i] != (byte)s[i]) { return false; }
  }
  return true;
}

static bool token_is_newline(span_cbyte_t text, struct source_token_t token) {
  return token.begin < token.end && token.begin < text.len
         && (text.data[token.begin] == '\n' || text.data[token.begin] == '\r');
}

static bool token_is_operatorish(span_cbyte_t text, struct source_token_t token) {
  if (token_empty(token) || token.begin >= text.len || token_eq(text, token, "@[")
      || token_eq(text, token, ":")) {
    return false;
  }
  for (size_t i = token.begin; i < token.end && i < text.len; ++i) {
    switch (text.data[i]) {
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
      case ':': break;
      default: return false;
    }
  }
  return true;
}

static error_t fmt_token_raw(fmt_t* fmt, span_cbyte_t text, struct source_token_t token) {
  for (size_t i = token.begin; i < token.end && i < text.len; ++i) {
    error_t err = fmt_byte(fmt, text.data[i]);
    if (err != ERROR_SUCCESS) { return err; }
  }
  return ERROR_SUCCESS;
}

static size_t token_escaped_len(span_cbyte_t text, struct source_token_t token) {
  size_t len = 2;
  for (size_t i = token.begin; i < token.end && i < text.len; ++i) {
    const byte c = text.data[i];
    len += (c == '\\' || c == '"' || c == '\n' || c == '\r' || c == '\t') ? 2U : 1U;
  }
  return len;
}

static error_t fmt_token_escaped(fmt_t* fmt, span_cbyte_t text, struct source_token_t token) {
  error_t err = fmt_byte(fmt, '"');
  if (err != ERROR_SUCCESS) { return err; }
  for (size_t i = token.begin; i < token.end && i < text.len; ++i) {
    const byte c = text.data[i];
    if (c == '\\' || c == '"') {
      err = fmt_byte(fmt, '\\');
      if (err != ERROR_SUCCESS) { return err; }
      err = fmt_byte(fmt, c);
    } else if (c == '\n') {
      err = fmt_text(fmt, "\\n");
    } else if (c == '\r') {
      err = fmt_text(fmt, "\\r");
    } else if (c == '\t') {
      err = fmt_text(fmt, "\\t");
    } else {
      err = fmt_byte(fmt, c);
    }
    if (err != ERROR_SUCCESS) { return err; }
  }
  return fmt_byte(fmt, '"');
}

static size_t sexpr_flat_len(span_cbyte_t text, const struct source_tree_t* tree, size_t index) {
  if (index >= source_tree_get_count(tree)) { return 0; }
  const struct source_node_t node = source_tree_get_node(tree, index);
  size_t len = 2 + cstr_len(source_node_type_t_str(node.type));
  if (node.type.value == SOURCE_NODE_TYPE_TOKEN) { len += 1 + token_escaped_len(text, node.token); }
  for (size_t child = node.child_index; child != TREE_NO_NODE;
       child = source_tree_get_node(tree, child).next_index) {
    len += 1 + sexpr_flat_len(text, tree, child);
  }
  return len;
}

static error_t sexpr_node(
    fmt_t* fmt, span_cbyte_t text, const struct source_tree_t* tree, size_t index, size_t depth) {
  if (index >= source_tree_get_count(tree)) { return ERROR_OUT_OF_BOUNDS; }
  const struct source_node_t node = source_tree_get_node(tree, index);
  if (fmt->col + sexpr_flat_len(text, tree, index) <= 100U
      && sexpr_flat_len(text, tree, index) <= 20U) {
    error_t err = fmt_byte(fmt, '(');
    if (err != ERROR_SUCCESS) { return err; }
    err = fmt_text(fmt, source_node_type_t_str(node.type));
    if (err != ERROR_SUCCESS) { return err; }
    if (node.type.value == SOURCE_NODE_TYPE_TOKEN) {
      err = fmt_byte(fmt, ' ');
      if (err != ERROR_SUCCESS) { return err; }
      err = fmt_token_escaped(fmt, text, node.token);
      if (err != ERROR_SUCCESS) { return err; }
    }
    for (size_t child = node.child_index; child != TREE_NO_NODE;
         child = source_tree_get_node(tree, child).next_index) {
      err = fmt_byte(fmt, ' ');
      if (err != ERROR_SUCCESS) { return err; }
      err = sexpr_node(fmt, text, tree, child, depth + 1);
      if (err != ERROR_SUCCESS) { return err; }
    }
    return fmt_byte(fmt, ')');
  }

  error_t err = fmt_byte(fmt, '(');
  if (err != ERROR_SUCCESS) { return err; }
  err = fmt_text(fmt, source_node_type_t_str(node.type));
  if (err != ERROR_SUCCESS) { return err; }
  if (node.type.value == SOURCE_NODE_TYPE_TOKEN) {
    err = fmt_byte(fmt, ' ');
    if (err != ERROR_SUCCESS) { return err; }
    err = fmt_token_escaped(fmt, text, node.token);
    if (err != ERROR_SUCCESS) { return err; }
  }
  for (size_t child = node.child_index; child != TREE_NO_NODE;
       child = source_tree_get_node(tree, child).next_index) {
    err = fmt_byte(fmt, '\n');
    if (err != ERROR_SUCCESS) { return err; }
    err = fmt_indent(fmt, (depth + 1) * SEXPR_INDENT);
    if (err != ERROR_SUCCESS) { return err; }
    err = sexpr_node(fmt, text, tree, child, depth + 1);
    if (err != ERROR_SUCCESS) { return err; }
  }
  if (node.child_index != TREE_NO_NODE) {
    err = fmt_byte(fmt, '\n');
    if (err != ERROR_SUCCESS) { return err; }
    err = fmt_indent(fmt, depth * SEXPR_INDENT);
    if (err != ERROR_SUCCESS) { return err; }
  }
  return fmt_byte(fmt, ')');
}

error_t source_tree_format_sexpr(
    span_cbyte_t text, const struct source_tree_t* tree, struct da_byte_t* out) {
  if (tree == NULL || out == NULL) { return ERROR_INVALID_PARAM; }
  if (source_tree_get_count(tree) == 0) { return fmt_text(&(fmt_t){.out = out, .col = 0}, "()"); }
  fmt_t fmt = {.out = out, .col = 0};
  error_t err = sexpr_node(&fmt, text, tree, 0, 0);
  if (err != ERROR_SUCCESS) { return err; }
  return fmt_byte(&fmt, '\n');
}

typedef struct canon_t {
  fmt_t fmt;
  bool need_space;
} canon_t;

static error_t canon_space(canon_t* c) {
  if (c->need_space && c->fmt.col != 0) {
    error_t err = fmt_byte(&c->fmt, ' ');
    if (err != ERROR_SUCCESS) { return err; }
  }
  c->need_space = false;
  return ERROR_SUCCESS;
}

static error_t canon_maybe_break(canon_t* c, size_t next_len) {
  if (c->fmt.col != 0 && c->fmt.col + next_len > CANON_LINE_MAX) {
    error_t err = fmt_text(&c->fmt, " ...\n");
    if (err != ERROR_SUCCESS) { return err; }
    c->need_space = false;
  }
  return ERROR_SUCCESS;
}

static error_t canon_token(canon_t* c, span_cbyte_t text, struct source_token_t token) {
  if (token_empty(token)) { return ERROR_SUCCESS; }
  const bool semicolon = token_eq(text, token, ";") || token_is_newline(text, token);
  const size_t len = semicolon ? 1 : token_len(text, token);

  if (semicolon) {
    error_t err = fmt_byte(&c->fmt, ';');
    if (err != ERROR_SUCCESS) { return err; }
    c->need_space = true;
    return ERROR_SUCCESS;
  }
  if (token_eq(text, token, ",")) {
    error_t err = fmt_text(&c->fmt, ", ");
    if (err != ERROR_SUCCESS) { return err; }
    c->need_space = false;
    return ERROR_SUCCESS;
  }
  if (token_eq(text, token, ":")) {
    error_t err = fmt_text(&c->fmt, ": ");
    if (err != ERROR_SUCCESS) { return err; }
    c->need_space = false;
    return ERROR_SUCCESS;
  }
  if (token_eq(text, token, ".") || token_eq(text, token, "(") || token_eq(text, token, "[")
      || token_eq(text, token, "@[") || token_eq(text, token, ")") || token_eq(text, token, "]")
      || (token.begin < token.end && token.begin < text.len && text.data[token.begin] == '{')) {
    error_t err = canon_maybe_break(c, len);
    if (err != ERROR_SUCCESS) { return err; }
    err = fmt_token_raw(&c->fmt, text, token);
    if (err != ERROR_SUCCESS) { return err; }
    c->need_space =
        token_eq(text, token, ")") || token_eq(text, token, "]")
        || (token.begin < token.end && token.begin < text.len && text.data[token.begin] == '{');
    return ERROR_SUCCESS;
  }
  if (token_is_operatorish(text, token)) {
    error_t err = canon_space(c);
    if (err != ERROR_SUCCESS) { return err; }
    err = canon_maybe_break(c, len + 1);
    if (err != ERROR_SUCCESS) { return err; }
    err = fmt_token_raw(&c->fmt, text, token);
    if (err != ERROR_SUCCESS) { return err; }
    err = fmt_byte(&c->fmt, ' ');
    if (err != ERROR_SUCCESS) { return err; }
    c->need_space = false;
    return ERROR_SUCCESS;
  }

  error_t err = canon_space(c);
  if (err != ERROR_SUCCESS) { return err; }
  err = canon_maybe_break(c, len);
  if (err != ERROR_SUCCESS) { return err; }
  err = fmt_token_raw(&c->fmt, text, token);
  if (err != ERROR_SUCCESS) { return err; }
  c->need_space = true;
  return ERROR_SUCCESS;
}

static error_t canon_node(
    canon_t* c, span_cbyte_t text, const struct source_tree_t* tree, size_t index) {
  if (index >= source_tree_get_count(tree)) { return ERROR_OUT_OF_BOUNDS; }
  const struct source_node_t node = source_tree_get_node(tree, index);
  if (node.type.value == SOURCE_NODE_TYPE_TOKEN) { return canon_token(c, text, node.token); }
  for (size_t child = node.child_index; child != TREE_NO_NODE;
       child = source_tree_get_node(tree, child).next_index) {
    error_t err = canon_node(c, text, tree, child);
    if (err != ERROR_SUCCESS) { return err; }
  }
  return ERROR_SUCCESS;
}

error_t source_tree_format_canonical(
    span_cbyte_t text, const struct source_tree_t* tree, struct da_byte_t* out) {
  if (tree == NULL || out == NULL) { return ERROR_INVALID_PARAM; }
  canon_t c = {.fmt = {.out = out, .col = 0}, .need_space = false};
  if (source_tree_get_count(tree) != 0) {
    error_t err = canon_node(&c, text, tree, 0);
    if (err != ERROR_SUCCESS) { return err; }
  }
  if (out->count > 0 && out->items[out->count - 1] != '\n') {
    error_t err = fmt_byte(&c.fmt, '\n');
    if (err != ERROR_SUCCESS) { return err; }
  }
  return ERROR_SUCCESS;
}
