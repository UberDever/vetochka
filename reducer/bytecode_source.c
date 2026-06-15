#include "bytecode_api.h"
#include "cells_api.h"
#include "source_api.h"
#include "vendor/stb_ds.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

#define BYTECODE_NO_NODE SIZE_MAX

typedef struct bytecode_source_encoder_t {
  span_cbyte_t text;
  const struct source_tree_t* source;
  struct bytecode_tree_builder_t* builder;
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

static bool source_token_span(bytecode_source_encoder_t* self, size_t index, span_cbyte_t* out) {
  struct source_node_t node = source_node(self, index);
  if (node.type.value != SOURCE_NODE_TYPE_TOKEN || node.token.begin > node.token.end
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

static size_t new_node0(bytecode_source_encoder_t* self, struct cells_node_t node) {
  return bytecode_new_node0(self->builder, node);
}

static size_t new_node2(
    bytecode_source_encoder_t* self, struct cells_node_t node, size_t left, size_t right) {
  return bytecode_new_node2(self->builder, node, left, right);
}

static size_t new_delta(bytecode_source_encoder_t* self) {
  return new_node0(self, cells_new_delta0());
}

static size_t new_i64(bytecode_source_encoder_t* self, i64 value) {
  return new_node0(self, cells_new_value0f(value));
}

static size_t new_bytes(bytecode_source_encoder_t* self, const byte* data, size_t len) {
  span_byte_t payload = {.data = (byte*)data, .len = len};
  return new_node0(self, cells_new_value0v(payload));
}

static size_t new_static_bytes(bytecode_source_encoder_t* self, const char* text) {
  return new_bytes(self, (const byte*)text, strlen(text));
}

static size_t new_apply(bytecode_source_encoder_t* self, size_t lhs, size_t rhs) {
  cells_node_type_t type = {.value = CELLS_NODE_TYPE_APPLY};
  return new_node2(self, cells_new_node(type), lhs, rhs);
}

static size_t new_list(bytecode_source_encoder_t* self, const size_t* items, size_t count) {
  size_t result = new_delta(self);
  while (count != 0) {
    count--;
    result = new_node2(self, cells_new_delta2(), items[count], result);
  }
  return result;
}

static size_t new_tagged(
    bytecode_source_encoder_t* self, const char* tag, const size_t* fields, size_t field_count) {
  size_t result = new_list(self, fields, field_count);
  return new_node2(self, cells_new_delta2(), new_static_bytes(self, tag), result);
}

// TODO: abstract this later and use VM info for that
static size_t new_op_fn(bytecode_source_encoder_t* self) {
  cells_node_type_t type = {.value = CELLS_NODE_TYPE_OP_FN0};
  return new_node0(self, cells_new_node(type));
}

static error_t parse_i64(span_cbyte_t token, i64* out) {
  if (token.len == 0 || out == NULL) { return ERROR_GENERIC; }
  i64 value = 0;
  for (size_t i = 0; i < token.len; i++) {
    byte c = token.data[i];
    if (c < '0' || c > '9') { return ERROR_GENERIC; }
    i64 digit = (i64)(c - '0');
    if (value > (INTPTR_MAX - digit) / 10) { return ERROR_OVERFLOW; }
    value = value * 10 + digit;
  }
  *out = value;
  return ERROR_SUCCESS;
}

static bool primary_is_opcode(bytecode_source_encoder_t* self, size_t index) {
  size_t token = source_child(self, index, 0);
  return token != BYTECODE_NO_NODE && source_token_eq(self, token, "{fn}");
}

static error_t encode_token(
    bytecode_source_encoder_t* self, size_t index, bool opcode_callee, size_t* out) {
  span_cbyte_t token = {0};
  if (!source_token_span(self, index, &token)) { return ERROR_GENERIC; }
  if (token.len == 1 && token.data[0] == '^') {
    *out = new_delta(self);
    return ERROR_SUCCESS;
  }
  if (token.len == 2 && token.data[0] == 0xce && token.data[1] == 0x94) {
    *out = new_delta(self);
    return ERROR_SUCCESS;
  }
  if (token.data[0] >= '0' && token.data[0] <= '9') {
    i64 value = 0;
    error_t err = parse_i64(token, &value);
    if (err != ERROR_SUCCESS) { return err; }
    *out = new_i64(self, value);
    return ERROR_SUCCESS;
  }
  if (token.len >= 2 && token.data[0] == '{' && token.data[token.len - 1] == '}') {
    if (opcode_callee && token.len == 4 && memcmp(token.data, "{fn}", 4) == 0) {
      *out = new_op_fn(self);
      return ERROR_SUCCESS;
    }
    *out = new_bytes(self, token.data + 1, token.len - 2);
    return ERROR_SUCCESS;
  }
  size_t fields[] = {new_bytes(self, token.data, token.len)};
  *out = new_tagged(self, ":id", fields, 1);
  return ERROR_SUCCESS;
}

static error_t encode_annotation_value(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  for (size_t child = source_node(self, index).child_index; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    if (source_node(self, child).type.value == SOURCE_NODE_TYPE_EXPRESSION) {
      return encode_node(self, child, out);
    }
  }
  return ERROR_GENERIC;
}

static error_t encode_expression_like(
    bytecode_source_encoder_t* self, size_t index, u8 body_type, size_t* out) {
  size_t annotation = BYTECODE_NO_NODE;
  size_t body = BYTECODE_NO_NODE;
  for (size_t child = source_node(self, index).child_index; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    u8 type = source_node(self, child).type.value;
    if (type == SOURCE_NODE_TYPE_ANNOTATION) { annotation = child; }
    if (type == body_type) { body = child; }
  }
  if (body == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
  error_t err = encode_node(self, body, out);
  if (err != ERROR_SUCCESS || annotation == BYTECODE_NO_NODE) { return err; }
  size_t annotation_value = 0;
  err = encode_annotation_value(self, annotation, &annotation_value);
  if (err != ERROR_SUCCESS) { return err; }
  size_t fields[] = {annotation_value, *out};
  *out = new_tagged(self, ":annot", fields, 2);
  return ERROR_SUCCESS;
}

static error_t encode_block(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t* items = NULL;
  error_t err = ERROR_SUCCESS;
  for (size_t child = source_node(self, index).child_index; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    if (source_node(self, child).type.value != SOURCE_NODE_TYPE_EXPRESSION) { continue; }
    size_t item = 0;
    err = encode_node(self, child, &item);
    if (err != ERROR_SUCCESS) { goto done; }
    stbds_arrput(items, item);
  }
  *out = new_tagged(self, ":block", items, stbds_arrlenu(items));

done:
  stbds_arrfree(items);
  return err;
}

static error_t encode_comma_items(
    bytecode_source_encoder_t* self, size_t index, size_t** out_items) {
  error_t err = ERROR_SUCCESS;
  for (size_t child = source_node(self, index).child_index; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    if (source_node(self, child).type.value != SOURCE_NODE_TYPE_EXPRESSION) { continue; }
    size_t item = 0;
    err = encode_node(self, child, &item);
    if (err != ERROR_SUCCESS) { return err; }
    stbds_arrput(*out_items, item);
  }
  return err;
}

static error_t encode_primary(
    bytecode_source_encoder_t* self, size_t index, bool opcode_callee, size_t* out) {
  size_t first = source_child(self, index, 0);
  if (first == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
  struct source_node_t first_node = source_node(self, first);
  if (first_node.type.value == SOURCE_NODE_TYPE_TOKEN) {
    size_t second = source_child(self, index, 1);
    if (source_token_eq(self, first, "[") && second != BYTECODE_NO_NODE) {
      size_t* items = NULL;
      error_t err = encode_comma_items(self, second, &items);
      if (err == ERROR_SUCCESS) { *out = new_tagged(self, ":list", items, stbds_arrlenu(items)); }
      stbds_arrfree(items);
      return err;
    }
    if (source_token_eq(self, first, "(") && second != BYTECODE_NO_NODE) {
      error_t err = encode_node(self, second, out);
      if (err != ERROR_SUCCESS) { return err; }
      size_t fields[] = {*out};
      *out = new_tagged(self, ":group", fields, 1);
      return ERROR_SUCCESS;
    }
    return encode_token(self, first, opcode_callee, out);
  }
  return encode_node(self, first, out);
}

static bool postfix_starts_application(bytecode_source_encoder_t* self, size_t index) {
  struct source_node_t node = source_node(self, index);
  if (node.type.value == SOURCE_NODE_TYPE_LOOSE_POSTFIX) { return true; }
  size_t first = source_child(self, index, 0);
  return first != BYTECODE_NO_NODE && !source_token_eq(self, first, ".");
}

static error_t encode_tight_postfix(
    bytecode_source_encoder_t* self, size_t index, size_t base, size_t* out) {
  size_t first = source_child(self, index, 0);
  if (first == BYTECODE_NO_NODE) { return ERROR_GENERIC; }

  if (source_token_eq(self, first, ".")) {
    size_t name = source_child(self, index, 1);
    span_cbyte_t token = {0};
    if (name == BYTECODE_NO_NODE || !source_token_span(self, name, &token)) {
      return ERROR_GENERIC;
    }
    size_t fields[] = {base, new_bytes(self, token.data, token.len)};
    *out = new_tagged(self, ":selector", fields, 2);
    return ERROR_SUCCESS;
  }

  if (source_token_eq(self, first, "(")) {
    size_t args = source_child(self, index, 1);
    if (args == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
    if (source_node(self, args).type.value == SOURCE_NODE_TYPE_IMPLICIT_DELTA) {
      *out = new_apply(self, base, new_delta(self));
      return ERROR_SUCCESS;
    }
    size_t* items = NULL;
    error_t err = encode_comma_items(self, args, &items);
    if (err == ERROR_SUCCESS) {
      *out = base;
      for (size_t i = 0; i < stbds_arrlenu(items); i++) {
        *out = new_apply(self, *out, items[i]);
      }
    }
    stbds_arrfree(items);
    return err;
  }

  if (source_token_eq(self, first, "[")) {
    size_t args = source_child(self, index, 1);
    size_t* items = NULL;
    error_t err = encode_comma_items(self, args, &items);
    if (err == ERROR_SUCCESS) {
      size_t argument = new_tagged(self, ":list", items, stbds_arrlenu(items));
      *out = new_apply(self, base, argument);
    }
    stbds_arrfree(items);
    return err;
  }

  span_cbyte_t token = {0};
  if (!source_token_span(self, first, &token) || token.len < 2 || token.data[0] != '{'
      || token.data[token.len - 1] != '}') {
    return ERROR_GENERIC;
  }
  size_t argument = new_bytes(self, token.data + 1, token.len - 2);
  *out = new_apply(self, base, argument);
  return ERROR_SUCCESS;
}

static error_t encode_block_argument(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  for (size_t child = source_node(self, index).child_index; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    if (source_node(self, child).type.value == SOURCE_NODE_TYPE_BLOCK_LIST) {
      return encode_block(self, child, out);
    }
  }
  size_t fields[] = {0};
  *out = new_tagged(self, ":block", fields, 0);
  return ERROR_SUCCESS;
}

static error_t encode_labeled_argument(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t name = source_child(self, index, 0);
  size_t argument = source_child(self, index, 2);
  span_cbyte_t name_token = {0};
  if (name == BYTECODE_NO_NODE || argument == BYTECODE_NO_NODE
      || !source_token_span(self, name, &name_token)) {
    return ERROR_GENERIC;
  }
  size_t value = 0;
  error_t err = encode_node(self, argument, &value);
  if (err != ERROR_SUCCESS) { return err; }
  size_t fields[] = {new_bytes(self, name_token.data, name_token.len), value};
  *out = new_tagged(self, ":label", fields, 2);
  return ERROR_SUCCESS;
}

static error_t encode_loose_postfix(
    bytecode_source_encoder_t* self, size_t index, size_t base, size_t* out) {
  size_t argument_node = source_child(self, index, 0);
  if (argument_node == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
  size_t argument = 0;
  u8 type = source_node(self, argument_node).type.value;
  error_t err = type == SOURCE_NODE_TYPE_BLOCK_ARGUMENT
                    ? encode_block_argument(self, argument_node, &argument)
                    : encode_labeled_argument(self, argument_node, &argument);
  if (err != ERROR_SUCCESS) { return err; }
  *out = new_apply(self, base, argument);
  return ERROR_SUCCESS;
}

static error_t encode_postfix(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t primary = source_child(self, index, 0);
  if (primary == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
  size_t first_postfix = source_node(self, primary).next_index;
  bool opcode_callee = first_postfix != BYTECODE_NO_NODE
                       && postfix_starts_application(self, first_postfix)
                       && primary_is_opcode(self, primary);
  error_t err = encode_primary(self, primary, opcode_callee, out);
  if (err != ERROR_SUCCESS) { return err; }

  for (size_t child = first_postfix; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    u8 type = source_node(self, child).type.value;
    if (type == SOURCE_NODE_TYPE_TIGHT_POSTFIX) {
      err = encode_tight_postfix(self, child, *out, out);
    } else if (type == SOURCE_NODE_TYPE_LOOSE_POSTFIX) {
      err = encode_loose_postfix(self, child, *out, out);
    } else {
      return ERROR_GENERIC;
    }
    if (err != ERROR_SUCCESS) { return err; }
  }
  return ERROR_SUCCESS;
}

static error_t encode_prefix(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t* operators = NULL;
  size_t operand = BYTECODE_NO_NODE;
  for (size_t child = source_node(self, index).child_index; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    if (source_node(self, child).type.value == SOURCE_NODE_TYPE_TOKEN) {
      stbds_arrput(operators, child);
    } else {
      operand = child;
    }
  }
  if (operand == BYTECODE_NO_NODE) {
    stbds_arrfree(operators);
    return ERROR_GENERIC;
  }
  error_t err = encode_node(self, operand, out);
  if (err != ERROR_SUCCESS) {
    stbds_arrfree(operators);
    return err;
  }
  for (size_t i = stbds_arrlenu(operators); i != 0; i--) {
    span_cbyte_t token = {0};
    if (!source_token_span(self, operators[i - 1], &token)) {
      err = ERROR_GENERIC;
      break;
    }
    size_t fields[] = {new_bytes(self, token.data, token.len), *out};
    *out = new_tagged(self, ":prefix", fields, 2);
  }
  stbds_arrfree(operators);
  return err;
}

static error_t encode_infix(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  size_t first = source_child(self, index, 0);
  if (first == BYTECODE_NO_NODE) { return ERROR_GENERIC; }
  size_t first_operand = 0;
  error_t err = encode_node(self, first, &first_operand);
  if (err != ERROR_SUCCESS) { return err; }

  size_t operator_node = source_node(self, first).next_index;
  if (operator_node == BYTECODE_NO_NODE) {
    *out = first_operand;
    return ERROR_SUCCESS;
  }
  span_cbyte_t operator_token = {0};
  if (!source_token_span(self, operator_node, &operator_token)) { return ERROR_GENERIC; }

  size_t* fields = NULL;
  stbds_arrput(fields, new_bytes(self, operator_token.data, operator_token.len));
  stbds_arrput(fields, first_operand);
  for (size_t op = operator_node; op != BYTECODE_NO_NODE;) {
    span_cbyte_t token = {0};
    if (!source_token_span(self, op, &token) || token.len != operator_token.len
        || memcmp(token.data, operator_token.data, token.len) != 0) {
      err = ERROR_GENERIC;
      goto done;
    }
    size_t operand_node = source_node(self, op).next_index;
    if (operand_node == BYTECODE_NO_NODE) {
      err = ERROR_GENERIC;
      goto done;
    }
    size_t operand = 0;
    err = encode_node(self, operand_node, &operand);
    if (err != ERROR_SUCCESS) { goto done; }
    stbds_arrput(fields, operand);
    op = source_node(self, operand_node).next_index;
  }
  *out = new_tagged(self, ":infix", fields, stbds_arrlenu(fields));

done:
  stbds_arrfree(fields);
  return err;
}

static error_t encode_source(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  for (size_t child = source_node(self, index).child_index; child != BYTECODE_NO_NODE;
       child = source_node(self, child).next_index) {
    if (source_node(self, child).type.value == SOURCE_NODE_TYPE_BLOCK_LIST) {
      return encode_block(self, child, out);
    }
  }
  size_t fields[] = {0};
  *out = new_tagged(self, ":block", fields, 0);
  return ERROR_SUCCESS;
}

static error_t encode_node(bytecode_source_encoder_t* self, size_t index, size_t* out) {
  if (index >= source_tree_get_count(self->source) || out == NULL) { return ERROR_OUT_OF_BOUNDS; }
  switch (source_node(self, index).type.value) {
    case SOURCE_NODE_TYPE_SOURCE: return encode_source(self, index, out);
    case SOURCE_NODE_TYPE_BLOCK_LIST: return encode_block(self, index, out);
    case SOURCE_NODE_TYPE_EXPRESSION:
      return encode_expression_like(self, index, SOURCE_NODE_TYPE_INFIX_EXPRESSION, out);
    case SOURCE_NODE_TYPE_ARGUMENT_EXPRESSION:
      return encode_expression_like(self, index, SOURCE_NODE_TYPE_INFIX_EXPRESSION_TIGHT, out);
    case SOURCE_NODE_TYPE_ANNOTATION: return encode_annotation_value(self, index, out);
    case SOURCE_NODE_TYPE_INFIX_EXPRESSION:
    case SOURCE_NODE_TYPE_INFIX_EXPRESSION_TIGHT: return encode_infix(self, index, out);
    case SOURCE_NODE_TYPE_PREFIX_EXPRESSION:
    case SOURCE_NODE_TYPE_PREFIX_EXPRESSION_TIGHT: return encode_prefix(self, index, out);
    case SOURCE_NODE_TYPE_POSTFIX_EXPRESSION:
    case SOURCE_NODE_TYPE_POSTFIX_EXPRESSION_TIGHT: return encode_postfix(self, index, out);
    case SOURCE_NODE_TYPE_PRIMARY: return encode_primary(self, index, false, out);
    case SOURCE_NODE_TYPE_COMMA_LIST: {
      size_t* items = NULL;
      error_t err = encode_comma_items(self, index, &items);
      if (err == ERROR_SUCCESS) { *out = new_tagged(self, ":list", items, stbds_arrlenu(items)); }
      stbds_arrfree(items);
      return err;
    }
    case SOURCE_NODE_TYPE_BLOCK_ARGUMENT: return encode_block_argument(self, index, out);
    case SOURCE_NODE_TYPE_LABELED_ARGUMENT: return encode_labeled_argument(self, index, out);
    case SOURCE_NODE_TYPE_TOKEN: return encode_token(self, index, false, out);
    case SOURCE_NODE_TYPE_IMPLICIT_DELTA: *out = new_delta(self); return ERROR_SUCCESS;
    default: return ERROR_GENERIC;
  }
}

error_t bytecode_source_encode(
    span_cbyte_t text,
    const struct source_tree_t* source,
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
