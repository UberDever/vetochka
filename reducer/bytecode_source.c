#include "bytecode_api.h"
#include "cells_api.h"
#include "source_api.h"
#include "vendor/stb_ds.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

// Lowering of a parsed source tree into inert terms, following the rewrite rules of
// docs/new_spec/02_syntax.md. Every tagged node is `[{:tag}, meta, ...payload]`;
// applications are data too: `[{@}, meta, f, x]`. Lists, nyads and literals carry no meta.

#define BYTECODE_NO_NODE SIZE_MAX

typedef struct bytecode_source_encoder_t {
  span_cbyte_t text;
  const struct source_tree_t* source;
  struct bytecode_tree_builder_t* builder;
  bool source_meta;
} bytecode_source_encoder_t;

static error_t encode_node(bytecode_source_encoder_t* self, size_t index, size_t* out);

static struct source_node_t source_node(bytecode_source_encoder_t* self, size_t index) {
  return source_tree_get_node(self->source, index);
}

static size_t source_child(bytecode_source_encoder_t* self, size_t index, size_t offset) {
  size_t child = source_node(self, index).child_index;
  while (child != BYTECODE_NO_NODE && offset != 0) {
    child = source_node(self, child).next_index;
    offset--;
  }
  return child;
}

static bool source_is_token(bytecode_source_encoder_t* self, size_t index) {
  const u8 type = source_node(self, index).type.value;
  return type == SOURCE_NODE_TYPE_TOKEN || type == SOURCE_NODE_TYPE_OP_PREFIX
         || type == SOURCE_NODE_TYPE_OP_INFIX;
}

static bool source_token_span(bytecode_source_encoder_t* self, size_t index, span_cbyte_t* out) {
  struct source_node_t node = source_node(self, index);
  if (!source_is_token(self, index) || node.token.begin > node.token.end
      || node.token.end > self->text.len) {
    return false;
  }
  *out = CTOR(
      span_cbyte_t,
      .data = self->text.data + node.token.begin,
      .len = node.token.end - node.token.begin);
  return true;
}

static bool source_token_eq(bytecode_source_encoder_t* self, size_t index, const char* text) {
  span_cbyte_t token = {0};
  size_t len = strlen(text);
  return source_token_span(self, index, &token) && token.len == len
         && memcmp(token.data, text, len) == 0;
}

// ---- term construction

static size_t new_nyad0(bytecode_source_encoder_t* self) {
  return bytecode_new_node0(self->builder, cells_new_delta0());
}

static size_t new_nyad1(bytecode_source_encoder_t* self, size_t x) {
  return bytecode_new_node1(self->builder, cells_new_delta1(), x);
}

static size_t new_nyad2(bytecode_source_encoder_t* self, size_t x, size_t y) {
  return bytecode_new_node2(self->builder, cells_new_delta2(), x, y);
}

static size_t new_i64(bytecode_source_encoder_t* self, i64 value) {
  return bytecode_new_node0(self->builder, cells_new_value0f(value));
}

static size_t new_bytes(bytecode_source_encoder_t* self, const byte* data, size_t len) {
  span_byte_t payload = {.data = (byte*)data, .len = len};
  return bytecode_new_node0(self->builder, cells_new_value0v(payload));
}

static size_t new_static_bytes(bytecode_source_encoder_t* self, const char* text) {
  return new_bytes(self, (const byte*)text, strlen(text));
}

static size_t new_span_bytes(bytecode_source_encoder_t* self, span_cbyte_t span) {
  return new_bytes(self, span.data, span.len);
}

// [a, b] -> ~[a, ~[b, ~[]]]
static size_t new_list(bytecode_source_encoder_t* self, const size_t* items, size_t count) {
  size_t result = new_nyad0(self);
  while (count != 0) {
    count--;
    result = new_nyad2(self, items[count], result);
  }
  return result;
}

// `[line: n, col: n]` when source positions are requested; `~[]` otherwise.
// Entries of a meta list carry empty meta themselves, so meta never recurses.
static size_t new_meta(bytecode_source_encoder_t* self, size_t source_index) {
  if (!self->source_meta) { return new_nyad0(self); }
  struct source_token_t token = source_node(self, source_index).token;
  size_t line[] = {
      new_static_bytes(self, ":label"),
      new_nyad0(self),
      new_static_bytes(self, "line"),
      new_i64(self, (i64)token.line)};
  size_t col[] = {
      new_static_bytes(self, ":label"),
      new_nyad0(self),
      new_static_bytes(self, "col"),
      new_i64(self, (i64)token.col)};
  size_t entries[] = {new_list(self, line, 4), new_list(self, col, 4)};
  return new_list(self, entries, 2);
}

// [{tag}, meta, ...fields]
static size_t new_tagged(
    bytecode_source_encoder_t* self,
    const char* tag,
    size_t source_index,
    const size_t* fields,
    size_t count) {
  size_t* items = NULL;
  stbds_arrput(items, new_static_bytes(self, tag));
  stbds_arrput(items, new_meta(self, source_index));
  for (size_t i = 0; i < count; i++) {
    stbds_arrput(items, fields[i]);
  }
  size_t result = new_list(self, items, stbds_arrlenu(items));
  stbds_arrfree(items);
  return result;
}

// f(x) -> {@} f x, as data: [{@}, meta, f, x]
static size_t new_application(
    bytecode_source_encoder_t* self, size_t source_index, size_t f, size_t x) {
  size_t fields[] = {f, x};
  return new_tagged(self, "@", source_index, fields, 2);
}

// ---- lowering

static error_t parse_i64(span_cbyte_t token, i64* out) {
  if (token.len == 0 || out == NULL) { return ERROR_GENERIC; }
  i64 value = 0;
  for (size_t i = 0; i < token.len; i++) {
    byte c = token.data[i];
    if (c < '0' || c > '9') { return ERROR_GENERIC; }
    i64 digit = (i64)(c - '0');
    if (value > (INT64_MAX - digit) / 10) { return ERROR_OVERFLOW; }
    value = value * 10 + digit;
  }
  *out = value;
  return ERROR_SUCCESS;
}

// Literals lower to themselves; `x -> [{:id}, meta, {x}]`; `$ -> [{:id}, meta, {$}]`.
static error_t encode_token(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  span_cbyte_t token = {0};
  if (!source_token_span(self, index, &token) || token.len == 0) { return ERROR_GENERIC; }
  if (token.data[0] >= '0' && token.data[0] <= '9') {
    i64 value = 0;
    error_t err = parse_i64(token, &value);
    if (err != ERROR_SUCCESS) { return err; }
    *out = new_i64(self, value);
    return ERROR_SUCCESS;
  }
  if (token.data[0] == '{') {
    *out = new_bytes(self, token.data + 1, token.len - 2);
    return ERROR_SUCCESS;
  }
  size_t fields[] = {new_span_bytes(self, token)};
  *out = new_tagged(self, ":id", index, fields, 1);
  return ERROR_SUCCESS;
}

// Encode every non-token child (entries of a comma or block list).
static error_t encode_entries(bytecode_source_encoder_t* self, size_t index, size_t** out_items) {
  if (index == BYTECODE_NO_NODE) { return ERROR_SUCCESS; }
  for (size_t child = source_node(self, index).child_index; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    if (source_is_token(self, child)) { continue; }
    size_t item = 0;
    error_t err = encode_node(self, child, &item);
    if (err != ERROR_SUCCESS) { return err; }
    stbds_arrput(*out_items, item);
  }
  return ERROR_SUCCESS;
}

// First child of `index` that is not a token, or BYTECODE_NO_NODE.
static size_t first_non_token(bytecode_source_encoder_t* self, size_t index) {
  for (size_t child = source_node(self, index).child_index; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    if (!source_is_token(self, child)) { return child; }
  }
  return BYTECODE_NO_NODE;
}

// [a, b] and the payload of f[a, b]
static error_t encode_list(bytecode_source_encoder_t* self, size_t comma_list, size_t* out) {
  size_t* items = NULL;
  error_t err = encode_entries(self, comma_list, &items);
  if (err == ERROR_SUCCESS) { *out = new_list(self, items, stbds_arrlenu(items)); }
  stbds_arrfree(items);
  return err;
}

// do a; b end -> [{:block}, meta, a, b]
static error_t encode_block(
    bytecode_source_encoder_t* self, size_t source_index, size_t block_list, size_t* out) {
  size_t* items = NULL;
  error_t err = encode_entries(self, block_list, &items);
  if (err == ERROR_SUCCESS) {
    *out = new_tagged(self, ":block", source_index, items, stbds_arrlenu(items));
  }
  stbds_arrfree(items);
  return err;
}

// ~[], ~[x], ~[x, y]
static error_t encode_nyad(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t* items = NULL;
  error_t err = encode_entries(self, index, &items);
  if (err == ERROR_SUCCESS) {
    switch (stbds_arrlenu(items)) {
      case 0: *out = new_nyad0(self); break;
      case 1: *out = new_nyad1(self, items[0]); break;
      case 2: *out = new_nyad2(self, items[0], items[1]); break;
      default: err = ERROR_GENERIC; break;
    }
  }
  stbds_arrfree(items);
  return err;
}

// label: expr -> [{:label}, meta, {label}, expr]
static error_t encode_labeled(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t name = source_child(self, index, 0);
  size_t argument = source_child(self, index, 2);
  span_cbyte_t name_token = {0};
  if (argument == BYTECODE_NO_NODE || !source_token_span(self, name, &name_token)) {
    return ERROR_GENERIC;
  }
  size_t value = 0;
  error_t err = encode_node(self, argument, &value);
  if (err != ERROR_SUCCESS) { return err; }
  size_t fields[] = {new_span_bytes(self, name_token), value};
  *out = new_tagged(self, ":label", index, fields, 2);
  return ERROR_SUCCESS;
}

// $fn: x -> {@} [{:id}, meta, {$}] [{:label}, meta, {fn}, x]
static error_t encode_opcode(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t dollar = source_child(self, index, 0);
  size_t labeled = source_child(self, index, 1);
  if (labeled == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
  size_t head = 0;
  error_t err = encode_token(self, dollar, &head);
  if (err != ERROR_SUCCESS) { return err; }
  size_t datum = 0;
  err = encode_node(self, labeled, &datum);
  if (err != ERROR_SUCCESS) { return err; }
  *out = new_application(self, index, head, datum);
  return ERROR_SUCCESS;
}

static error_t encode_primary(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t first = source_child(self, index, 0);
  if (first == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
  if (source_node(self, first).type.value == SOURCE_NODE_TYPE_OPCODE) {
    return encode_opcode(self, first, out);
  }
  if (source_token_eq(self, first, "[")) {
    return encode_list(self, first_non_token(self, index), out);
  }
  if (source_token_eq(self, first, "(")) {
    // (entry) -> entry: parens are purely syntactic.
    return encode_node(self, first_non_token(self, index), out);
  }
  return encode_token(self, first, out);
}

static error_t encode_tight_postfix(
    bytecode_source_encoder_t* self, size_t index, size_t base, size_t* out) {
  size_t first = source_child(self, index, 0);
  size_t second = source_child(self, index, 1);
  if (first == BYTECODE_NO_NODE) { return ERROR_GENERIC; }

  // base.name -> [{:selector}, meta, base, {name}]
  if (source_token_eq(self, first, ".")) {
    span_cbyte_t name = {0};
    if (second == BYTECODE_NO_NODE || !source_token_span(self, second, &name)) {
      return ERROR_GENERIC;
    }
    size_t fields[] = {base, new_span_bytes(self, name)};
    *out = new_tagged(self, ":selector", index, fields, 2);
    return ERROR_SUCCESS;
  }

  // f(x, y) -> {@} ({@} f x) y; f() == f(~[])
  if (source_token_eq(self, first, "(")) {
    if (second == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
    if (source_node(self, second).type.value == SOURCE_NODE_TYPE_IMPLICIT_NYAD) {
      *out = new_application(self, index, base, new_nyad0(self));
      return ERROR_SUCCESS;
    }
    size_t* items = NULL;
    error_t err = encode_entries(self, second, &items);
    if (err == ERROR_SUCCESS) {
      *out = base;
      for (size_t i = 0; i < stbds_arrlenu(items); i++) {
        *out = new_application(self, index, *out, items[i]);
      }
    }
    stbds_arrfree(items);
    return err;
  }

  // f[x, y] -> {@} f ~[x, ~[y, ~[]]]
  if (source_token_eq(self, first, "[")) {
    size_t argument = 0;
    error_t err = encode_list(self, first_non_token(self, index), &argument);
    if (err != ERROR_SUCCESS) { return err; }
    *out = new_application(self, index, base, argument);
    return ERROR_SUCCESS;
  }

  // f{bytes} -> {@} f {bytes}
  size_t argument = 0;
  error_t err = encode_token(self, first, &argument);
  if (err != ERROR_SUCCESS) { return err; }
  *out = new_application(self, index, base, argument);
  return ERROR_SUCCESS;
}

// primary tight_postfix* loose_postfix*; a loose postfix is plain application of its datum.
static error_t encode_postfix(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t primary = source_child(self, index, 0);
  if (primary == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
  error_t err = encode_node(self, primary, out);
  if (err != ERROR_SUCCESS) { return err; }

  for (size_t child = source_node(self, primary).next_index; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    u8 type = source_node(self, child).type.value;
    if (type == SOURCE_NODE_TYPE_TIGHT_POSTFIX) {
      err = encode_tight_postfix(self, child, *out, out);
    } else if (type == SOURCE_NODE_TYPE_LOOSE_POSTFIX) {
      size_t datum = 0;
      err = encode_node(self, source_child(self, child, 0), &datum);
      if (err == ERROR_SUCCESS) { *out = new_application(self, child, *out, datum); }
    } else {
      err = ERROR_GENERIC;
    }
    if (err != ERROR_SUCCESS) { return err; }
  }
  return ERROR_SUCCESS;
}

// prefix-op expr -> [{:prefix}, meta, {op}, expr]
static error_t encode_prefix(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t operand = first_non_token(self, index);
  if (operand == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
  error_t err = encode_node(self, operand, out);
  if (err != ERROR_SUCCESS) { return err; }

  size_t* operators = NULL;
  for (size_t child = source_node(self, index).child_index; child != operand;
       child = source_node(self, child).next_index) {
    stbds_arrput(operators, child);
  }
  for (size_t i = stbds_arrlenu(operators); i != 0; i--) {
    span_cbyte_t token = {0};
    if (!source_token_span(self, operators[i - 1], &token)) {
      err = ERROR_GENERIC;
      break;
    }
    size_t fields[] = {new_span_bytes(self, token), *out};
    *out = new_tagged(self, ":prefix", index, fields, 2);
  }
  stbds_arrfree(operators);
  return err;
}

// x op1 y op2 z -> [{:infix}, meta, [{op1}, {op2}], x, y, z]
static error_t encode_infix(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t first = source_child(self, index, 0);
  if (first == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
  if (source_node(self, first).next_index == BYTECODE_NO_NODE) {
    return encode_node(self, first, out);
  }

  size_t* operators = NULL;
  size_t* fields = NULL;
  error_t err = ERROR_SUCCESS;
  stbds_arrput(fields, 0); // operator list, filled below
  for (size_t child = first; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    span_cbyte_t token = {0};
    if (source_token_span(self, child, &token)) {
      stbds_arrput(operators, new_span_bytes(self, token));
      continue;
    }
    size_t operand = 0;
    err = encode_node(self, child, &operand);
    if (err != ERROR_SUCCESS) { goto done; }
    stbds_arrput(fields, operand);
  }
  fields[0] = new_list(self, operators, stbds_arrlenu(operators));
  *out = new_tagged(self, ":infix", index, fields, stbds_arrlenu(fields));

done:
  stbds_arrfree(operators);
  stbds_arrfree(fields);
  return err;
}

// annotation? infix; @[a, b] expr -> [{:annot}, meta, ~[a, ~[b, ~[]]], expr]
static error_t encode_expression(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t annotation = BYTECODE_NO_NODE;
  size_t body = BYTECODE_NO_NODE;
  for (size_t child = source_node(self, index).child_index; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    if (source_node(self, child).type.value == SOURCE_NODE_TYPE_ANNOTATION) {
      annotation = child;
    } else {
      body = child;
    }
  }
  if (body == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
  error_t err = encode_node(self, body, out);
  if (err != ERROR_SUCCESS || annotation == BYTECODE_NO_NODE) { return err; }
  size_t annotations = 0;
  err = encode_list(self, first_non_token(self, annotation), &annotations);
  if (err != ERROR_SUCCESS) { return err; }
  size_t fields[] = {annotations, *out};
  *out = new_tagged(self, ":annot", index, fields, 2);
  return ERROR_SUCCESS;
}

static error_t encode_node(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  if (index >= source_tree_get_count(self->source) || out == NULL) { return ERROR_OUT_OF_BOUNDS; }
  switch (source_node(self, index).type.value) {
    case SOURCE_NODE_TYPE_SOURCE:
    case SOURCE_NODE_TYPE_BLOCK_ARGUMENT:
      return encode_block(self, index, first_non_token(self, index), out);
    case SOURCE_NODE_TYPE_BLOCK_LIST: return encode_block(self, index, index, out);
    case SOURCE_NODE_TYPE_EXPRESSION:
    case SOURCE_NODE_TYPE_ARGUMENT_EXPRESSION: return encode_expression(self, index, out);
    case SOURCE_NODE_TYPE_INFIX_EXPRESSION:
    case SOURCE_NODE_TYPE_INFIX_EXPRESSION_TIGHT: return encode_infix(self, index, out);
    case SOURCE_NODE_TYPE_PREFIX_EXPRESSION:
    case SOURCE_NODE_TYPE_PREFIX_EXPRESSION_TIGHT: return encode_prefix(self, index, out);
    case SOURCE_NODE_TYPE_POSTFIX_EXPRESSION:
    case SOURCE_NODE_TYPE_POSTFIX_EXPRESSION_TIGHT: return encode_postfix(self, index, out);
    case SOURCE_NODE_TYPE_PRIMARY: return encode_primary(self, index, out);
    case SOURCE_NODE_TYPE_NYAD: return encode_nyad(self, index, out);
    case SOURCE_NODE_TYPE_OPCODE: return encode_opcode(self, index, out);
    case SOURCE_NODE_TYPE_LABELED_EXPRESSION: return encode_labeled(self, index, out);
    case SOURCE_NODE_TYPE_IMPLICIT_NYAD: *out = new_nyad0(self); return ERROR_SUCCESS;
    case SOURCE_NODE_TYPE_TOKEN: return encode_token(self, index, out);
    default: return ERROR_GENERIC;
  }
}

error_t bytecode_source_encode(
    span_cbyte_t text,
    const struct source_tree_t* source,
    struct bytecode_source_options_t options,
    struct cells_t* cells,
    size_t* index_out) {
  if (source == NULL || cells == NULL || index_out == NULL
      || (text.len != 0 && text.data == NULL)) {
    return ERROR_INVALID_PARAM;
  }
  if (source_tree_get_count(source) == 0) { return ERROR_GENERIC; }

  struct bytecode_tree_builder_t* builder = NULL;
  error_t err = bytecode_tree_builder_create(&builder);
  if (err != ERROR_SUCCESS) { return err; }
  bytecode_source_encoder_t encoder = {
      .text = text,
      .source = source,
      .builder = builder,
      .source_meta = options.source_meta,
  };
  size_t root = 0;
  err = encode_node(&encoder, 0, &root);
  if (err == ERROR_SUCCESS) {
    (void)root;
    err = bytecode_tree_builder_build(builder, cells, index_out);
  }
  bytecode_tree_builder_destroy(&builder);
  return err;
}
